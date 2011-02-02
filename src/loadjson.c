
/****
  © 2011 Basile Starynkevitch 
    this file loadjson.c is part of IaCa 
                          [loading data in JSON format thru YAJL]

    IaCa is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
  
    IaCa is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
  
    You should have received a copy of the GNU General Public License
    along with IaCa.  If not, see <http://www.gnu.org/licenses/>.
****/

#include "iaca.h"


/* the JSON loader has a stack of values and a stack of states. Each state is */
#define IACAJS_STATES_LIST()			\
  IACAJS_STATE(IACAJS__NONE),			\
  IACAJS_STATE(IACAJS_WAITVAL),			\
  IACAJS_STATE(IACAJS_WAITINTVAL),     		\
  IACAJS_STATE(IACAJS_WAITKIND),		\
  IACAJS_STATE(IACAJS_WAITNODESONS),		\
  IACAJS_STATE(IACAJS_WAITNODECONN),		\
  IACAJS_STATE(IACAJS_WAITITEMID),		\
  IACAJS_STATE(IACAJS_GOTVAL),			\
  IACAJS_STATE(IACAJS_GOTNODESONS),		\
  IACAJS_STATE(IACAJS__LAST)

enum iacajsonstate_en
{
#define IACAJS_STATE(St) St
  IACAJS_STATES_LIST ()
#undef IACAJS_STATE
};

static const char *iacajs_state_names[] = {
#define IACAJS_STATE(St) #St
  IACAJS_STATES_LIST (),
  (char *) 0
};

struct iacajsonstate_st
{
  enum iacajsonstate_en js_state;
  IacaValue *js_val;
  union
  {
    void *js_ptr;
    long js_num;
    char *js_str;
    struct
    {
      IacaItem *js_node_conn;
      IacaValue **js_node_sons;
      int js_node_arity;
      int js_node_sizesons;
    };
  };
};

struct iacaloader_st
{
  /* Glib hash table asociating item identifiers to loaded item, actually
     the key is a IacaItem structure ... */
  GHashTable *ld_itemhtab;
  /* the handle to the YAJL JSON parser. See git://github.com/lloyd/yajl */
  yajl_handle ld_json;
  /* Glib array, stack of struct iacajsonstate_st */
  GArray *ld_stack;
};

/* a Glib GHashTable compatible hash function on items */
guint
iaca_item_ghash (gconstpointer p)
{
  const IacaItem *itm = (const IacaItem *) p;
  if (!itm)
    return 0;
  return itm->v_ident % 1073741939;	/* a prime number near 1 << 30 */
}

gboolean
iaca_item_gheq (gconstpointer p1, gconstpointer p2)
{
  const IacaItem *itm1 = (const IacaItem *) p1;
  const IacaItem *itm2 = (const IacaItem *) p1;
  if (itm1 == itm2)
    return TRUE;
  if (!itm1 || !itm2)
    return FALSE;
  return itm1->v_ident == itm2->v_ident;
}


/* retrieve or create a loaded item of given id */
static inline IacaItem *
iaca_retrieve_loaded_item (struct iacaloader_st *ld, int64_t id)
{
  IacaItem *itm = 0;
  struct iacaitem_st itmst = { 0 };
  if (id <= 0 || !ld || !ld->ld_itemhtab)
    return NULL;
  itmst.v_ident = id;
  itm = g_hash_table_lookup (ld->ld_itemhtab, &itmst);
  if (itm)
    return itm;
  itm = iaca_alloc_data (sizeof (IacaItem));
  itm->v_kind = IACAV_ITEM;
  itm->v_ident = id;
  if (iaca_item_last_ident < id)
    iaca_item_last_ident = id;
  itm->v_attrtab = NULL;
  itm->v_payloadkind = IACAPAYLOAD__NONE;
  itm->v_payloadptr = NULL;
  itm->v_itemcontent = NULL;
  g_hash_table_insert (ld->ld_itemhtab, itm, itm);
  return itm;
}

static inline struct iacajsonstate_st *
iaca_json_state (struct iacaloader_st *ld, unsigned n)
{
  int ln = 0;
  GArray *sta = 0;
  if (!ld)
    return NULL;
  sta = ld->ld_stack;
  if (!sta)
    return NULL;
  ln = sta->len;
  if (ln <= 0 || n > ln)
    return NULL;
  return &g_array_index (sta, struct iacajsonstate_st, ln - n);
}

#define iaca_json_top_state(Ld) iaca_json_state((Ld),1)

#define iaca_json_state_push(Ld,St) iaca_json_state_push_at(__LINE__,(Ld),(St))
static inline void
iaca_json_state_push_at (int lin, struct iacaloader_st *ld,
			 enum iacajsonstate_en ste)
{
  GArray *sta = 0;
  struct iacajsonstate_st newst = { 0 };
  if (!ld)
    return;
  sta = ld->ld_stack;
  if (!sta)
    return;
  memset (&newst, 0, sizeof (newst));
  newst.js_state = ste;
  iaca_debug_at (__FILE__, lin, "push state [%d] #%d %s", sta->len,
		 (int) ste, iacajs_state_names[(int) ste]);
  g_array_append_val (sta, newst);
}

#define iaca_json_state_pop(Ld) iaca_json_state_pop_at(__LINE__,(Ld))
static inline void
iaca_json_state_pop_at (int lin, struct iacaloader_st *ld)
{
  int ln = 0;
  GArray *sta = 0;
  if (!ld)
    return;
  sta = ld->ld_stack;
  if (!sta)
    return;
  ln = sta->len;
  if (ln <= 0)
    return;
  iaca_debug_at (__FILE__, lin, "pop state [%d]", ln);
  g_array_remove_index (sta, ln - 1);
}


static inline void
iaca_json_state_add_node_son (struct iacajsonstate_st *topst,
			      IacaValue *newson)
{
  g_assert (topst && topst->js_state == IACAJS_WAITNODESONS);
  if (topst->js_node_arity >= topst->js_node_sizesons)
    {
      int newsize = 8 + 3 * topst->js_node_sizesons / 2;
      IacaValue **newsons = iaca_alloc_data (newsize * sizeof (IacaValue *));
      if (topst->js_node_sons)
	for (int i = topst->js_node_arity - 1; i >= 0; i--)
	  newsons[i] = topst->js_node_sons[i];
      GC_FREE (topst->js_node_sons);
      topst->js_node_sons = newsons;
      topst->js_node_sizesons = newsize;
    }
  topst->js_node_sons[topst->js_node_arity] = newson;
  topst->js_node_arity++;
}

#define iaca_json_got_value(Ld,Va) iaca_json_got_value_at(__LINE__,(Ld),(Va))
bool
iaca_json_got_value_at (int lin, struct iacaloader_st *ld, IacaValue *val)
{
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  iaca_debug ("lin %d start topst %p statedepth %d val@ %p", lin, topst,
	      ld->ld_stack ? ld->ld_stack->len : 0, val);
  if (!topst)
    {
      iaca_debug ("empty stack fail");
      return false;
    }
  iaca_debug ("top state #%d [%s]", (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST]);
  switch (topst->js_state)
    {
    case IACAJS_WAITNODESONS:
      iaca_json_state_add_node_son (topst, val);
      iaca_debug ("added son @ %p", val);
      return true;
    default:
      iaca_debug ("fail");
      return false;
    }
}

/* various YAJL handlers for loading JSON */

static int
iacaloadyajl_null (void *ctx)
{
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  iaca_debug ("start topst %p statedepth %d", topst,
	      ld->ld_stack ? ld->ld_stack->len : 0);
  if (!topst)
    return 0;
  iaca_debug ("state #%d [%s]", (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST]);
  switch (topst->js_state)
    {
    case IACAJS_WAITVAL:
      topst->js_val = NULL;
      topst->js_state = IACAJS_GOTVAL;
      iaca_debug ("got null");
      return 1;
    default:
      iaca_debug ("fail");
      return 0;
    }
}

static int
iacaloadyajl_boolean (void *ctx, int b)
{
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  iaca_debug ("start topst %p statedepth %d", topst,
	      ld->ld_stack ? ld->ld_stack->len : 0);
  if (!topst)
    return 0;
  iaca_debug (" state #%d [%s]", (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST]);
  iaca_debug ("fail");
  return 0;
}

static int
iacaloadyajl_number (void *ctx, const char *s, unsigned slen)
{
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  long long ll = 0;
  char *end = 0;
  iaca_debug ("start topst %p statedepth %d", topst,
	      ld->ld_stack ? ld->ld_stack->len : 0);
  if (!topst)
    return 0;
  ll = strtoll (s, &end, 0);
  iaca_debug ("state #%d [%s] ll %lld", (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST], ll);
  switch (topst->js_state)
    {
    case IACAJS_WAITNODECONN:
      {
	IacaItem *itm = iaca_retrieve_loaded_item (ld, ll);
	if (topst->js_node_conn)
	  {
	    iaca_debug ("fail already got conn");
	    return 0;
	  }
	iaca_debug ("connective item #%lld %p", ll, itm);
	topst->js_node_conn = itm;
	topst->js_state = IACAJS_WAITNODESONS;
	return 1;
      }
    case IACAJS_WAITINTVAL:
      {
	IacaValue *ival = iacav_integer_make (ll);
	iaca_debug ("got ival @ %p %ld", ival, (long) ll);
	topst->js_state = IACAJS_GOTVAL;
	topst->js_val = ival;
	return 1;
      }
    case IACAJS_WAITITEMID:
      {
	IacaItem *itm = iaca_retrieve_loaded_item (ld, ll);
	iaca_debug ("got itm val %p #%lld", itm, ll);
	topst->js_state = IACAJS_GOTVAL;
	topst->js_val = (IacaValue *) itm;
	return 1;
      }
    default:
      iaca_debug ("fail");
      return 0;
    }
  return 0;
}


static int
iacaloadyajl_string (void *ctx, const unsigned char *str, unsigned slen)
{
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  iaca_debug ("start topst %p statedepth %d", topst,
	      ld->ld_stack ? ld->ld_stack->len : 0);
  if (!topst)
    return 0;
  iaca_debug ("state #%d [%s] str %.*s", (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST], slen, str);
  switch (topst->js_state)
    {
    case IACAJS_WAITKIND:
      if (!strncmp ("itrv", (const char *) str, slen))
	{
	  topst->js_state = IACAJS_WAITITEMID;
	  iaca_debug ("waititemid");
	  return 1;
	}
      if (!strncmp ("intv", (const char *) str, slen))
	{
	  topst->js_state = IACAJS_WAITINTVAL;
	  iaca_debug ("waitintval");
	  return 1;
	}
      else if (!strncmp ("nodv", (const char *) str, slen))
	{
	  topst->js_state = IACAJS_WAITNODECONN;
	  topst->js_node_conn = 0;
	  topst->js_node_sons = 0;
	  topst->js_node_arity = 0;
	  topst->js_node_sizesons = 0;
	  iaca_debug ("waitnodeconn");
	  return 1;
	}
    default:
      iaca_debug ("fail");
      return 0;
    }
}

static int
iacaloadyajl_map_key (void *ctx, const unsigned char *str, unsigned int slen)
{
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  iaca_debug ("start topst %p statedepth %d", topst,
	      ld->ld_stack ? ld->ld_stack->len : 0);
  if (!topst)
    return 0;
  iaca_debug ("state #%d [%s] key %.*s",
	      (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST], slen, str);
  switch (topst->js_state)
    {
    case IACAJS_WAITKIND:
      if (!strncmp ("kd", (const char *) str, slen))
	{
	  iaca_debug ("got kd");
	  return 1;
	}
      break;
    case IACAJS_WAITITEMID:
      if (!strncmp ("id", (const char *) str, slen))
	{
	  iaca_debug ("got id");
	  return 1;
	}
      break;
    case IACAJS_WAITNODECONN:
      if (!strncmp ("conid", (const char *) str, slen))
	{
	  iaca_debug ("got conid");
	  return 1;
	}
      break;
    case IACAJS_WAITNODESONS:
      if (!strncmp ("sons", (const char *) str, slen))
	{
	  iaca_debug ("got sons");
	  return 1;
	}
      break;
    case IACAJS_WAITINTVAL:
      if (!strncmp ("int", (const char *) str, slen))
	{
	  iaca_debug ("got int");
	  return 1;
	}
      break;
    default:
      break;
    }
  iaca_debug ("fail");
  return 0;
}


static int
iacaloadyajl_start_map (void *ctx)
{
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  iaca_debug ("start topst %p statedepth %d", topst,
	      ld->ld_stack ? ld->ld_stack->len : 0);
  if (!topst)
    return 0;
  iaca_debug ("state #%d [%s]", (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST]);
  switch (topst->js_state)
    {
    case IACAJS_WAITVAL:
      iaca_debug ("waiting kind");
      topst->js_state = IACAJS_WAITKIND;
      return 1;
    case IACAJS_WAITNODESONS:
      iaca_json_state_push (ld, IACAJS_WAITKIND);
      iaca_debug ("waiting kind for son");
      return 1;
    default:
      iaca_debug ("fail");
      return 0;
    }
}

static int
iacaloadyajl_end_map (void *ctx)
{
  IacaValue *val = 0;
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  iaca_debug ("start topst %p statedepth %d", topst,
	      ld->ld_stack ? ld->ld_stack->len : 0);
  if (!topst)
    return 0;
  iaca_debug ("state #%d [%s]", (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST]);
  switch (topst->js_state)
    {
    case IACAJS_GOTVAL:
    set_value:
      {
	val = topst->js_val;
	iaca_debug ("got val %p", val);
	iaca_json_state_pop (ld);
	topst = iaca_json_top_state (ld);
	if (!topst)
	  {
	    iaca_debug ("fail empty stack");
	    return 0;
	  }
	iaca_debug ("now top state #%d [%s]", (int) topst->js_state,
		    iacajs_state_names[topst->js_state % IACAJS__LAST]);
	if (!iaca_json_got_value (ld, val))
	  {
	    iaca_debug ("fail got val");
	    return 0;
	  };
	iaca_debug ("done val %p", val);
	return 1;
      }
    case IACAJS_WAITINTVAL:
      {
	IacaValue *val = iacav_integer_make (topst->js_num);
	iaca_debug ("made int val %ld @%p", topst->js_num, val);
	return 1;
      }
    case IACAJS_WAITNODESONS:
      {
	IacaValue *val = 0;
	val = topst->js_val;
	iaca_debug ("son #%d %p", topst->js_node_arity, val);
	iaca_json_state_add_node_son (topst, val);
	return 1;
      }
    case IACAJS_GOTNODESONS:
      {
	IacaValue* val = 0;
	g_assert (topst->js_node_conn != 0);
	iaca_debug("making node of conn %p and arity %d", 
		   topst->js_node_conn, topst->js_node_arity);
	topst->js_val = val 
	  = iacav_node_make((IacaValue*)topst->js_node_conn, 
			    topst->js_node_sons, topst->js_node_arity);
	goto set_value;
      }
    default:
      iaca_debug ("fail");
      return 0;
    }
}


static int
iacaloadyajl_start_array (void *ctx)
{
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  iaca_debug ("start topst %p statedepth %d", topst,
	      ld->ld_stack ? ld->ld_stack->len : 0);
  if (!topst)
    return 0;
  iaca_debug ("state #%d [%s]", (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST]);
  switch (topst->js_state)
    {
    case IACAJS_WAITNODESONS:
      if (topst->js_node_sizesons != 0)
	{
	  iaca_debug ("sizesons %d", topst->js_node_sizesons);
	  return 0;
	}
      topst->js_node_arity = 0;
      topst->js_node_sizesons = 4;
      topst->js_node_sons =
	iaca_alloc_data (sizeof (IacaValue *) * topst->js_node_sizesons);
      iaca_json_state_push (ld, IACAJS_WAITVAL);
      return 1;
    default:
      return 0;
    }
  return 0;
}

static int
iacaloadyajl_end_array (void *ctx)
{
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  iaca_debug ("start topst %p statedepth %d", topst,
	      ld->ld_stack ? ld->ld_stack->len : 0);
  if (!topst)
    return 0;
  iaca_debug ("state #%d [%s]", (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST]);
  switch (topst->js_state)
    {
    case IACAJS_WAITNODESONS:
      {
	topst->js_state = IACAJS_GOTNODESONS;
	iaca_debug ("gotsons arity %d", topst->js_node_arity);
	return 1;
      }
    default:
      iaca_debug("failed");
      return 0;
    }
}



static const yajl_callbacks iacaloadyajl_callbacks = {
  .yajl_null = iacaloadyajl_null,
  .yajl_boolean = iacaloadyajl_boolean,
  .yajl_number = iacaloadyajl_number,
  .yajl_string = iacaloadyajl_string,
  .yajl_start_map = iacaloadyajl_start_map,
  .yajl_map_key = iacaloadyajl_map_key,
  .yajl_end_map = iacaloadyajl_end_map,
  .yajl_start_array = iacaloadyajl_start_array,
  .yajl_end_array = iacaloadyajl_end_array
};

static void
iaca_yajl_free (void *ctx, void *ptr)
{
  GC_FREE (ptr);
}

static void *
iaca_yajl_malloc (void *ctx, unsigned sz)
{
  void *ptr = 0;
  ptr = iaca_alloc_data (sz);
  return ptr;
}

static void *
iaca_yajl_realloc (void *ctx, void *ptr, unsigned sz)
{
  ptr = GC_REALLOC (ptr, sz);
  return ptr;
}

static const yajl_alloc_funcs iaca_yajl_alloc_functions = {
  .malloc = iaca_yajl_malloc,
  .free = iaca_yajl_free,
  .realloc = iaca_yajl_realloc,
};

static const yajl_parser_config iaca_yajl_parserconf = {.allowComments =
    1,.checkUTF8 = 1
};

void
iaca_load_data (struct iacaloader_st *ld, const char *datapath)
{
  FILE *datafil = 0;
  char *line = 0;
  size_t linsz = 0;
  int linenum = 0;
  yajl_status stat = yajl_status_ok;
  g_assert (ld != NULL);
  g_assert (ld->ld_json == NULL);
  datafil = fopen (datapath, "r");
  if (!datafil)
    iaca_error ("failed to open data file %s - %m", datapath);
  ld->ld_json =
    yajl_alloc (&iacaloadyajl_callbacks, &iaca_yajl_parserconf,
		&iaca_yajl_alloc_functions, ld);
  ld->ld_stack = g_array_new (TRUE, TRUE, sizeof (struct iacajsonstate_st));
  linsz = 256;
  line = calloc (linsz, 1);
  iaca_json_state_push (ld, IACAJS_WAITVAL);
  while (!feof (datafil))
    {
      ssize_t linlen = getline (&line, &linsz, datafil);
      linenum++;
      stat = yajl_parse (ld->ld_json, (const unsigned char *) line, linlen);
      if (stat != yajl_status_ok && stat != yajl_status_insufficient_data)
	iaca_error ("in file %s line %d YAJL JSON parse error: %s",
		    datapath, (int) linenum,
		    yajl_get_error (ld->ld_json, TRUE,
				    (const unsigned char *) line, linlen));
    };
  stat = yajl_parse_complete (ld->ld_json);
  if (stat != yajl_status_ok)
    iaca_error ("YAJL parse error at end of %s: %s",
		datapath, yajl_get_error (ld->ld_json, FALSE,
					  (const unsigned char *) 0, 0));
  fclose (datafil);
  free (line);
  yajl_free (ld->ld_json);
  g_array_free (ld->ld_stack, TRUE);
  ld->ld_stack = 0;
  ld->ld_json = 0;
  iaca_debug ("successfully loaded data file %s", datapath);
}

void
iaca_load (const char *dirpath)
{
  gchar *manipath = 0;
  struct iacaloader_st ld = { 0 };
  char *line = 0;
  size_t siz = 0;
  FILE *fil = 0;
  if (!dirpath || !dirpath[0])
    dirpath = ".";
  manipath = g_build_filename (dirpath, IACA_MANIFEST_FILE, NULL);
  memset (&ld, 0, sizeof (ld));
  ld.ld_itemhtab = g_hash_table_new (iaca_item_ghash, iaca_item_gheq);
  fil = fopen (manipath, "r");
  if (!fil)
    iaca_error ("failed to open manifest file %s - %m", manipath);
  siz = 80;
  line = calloc (siz, 1);
  iaca_module_htab = g_hash_table_new (g_str_hash, g_str_equal);
  iaca_data_htab = g_hash_table_new (g_str_hash, g_str_equal);
  while (!feof (fil))
    {
      ssize_t linlen = getline (&line, &siz, fil);
      char *name = 0;
      // skip comment or empty line in manifest
      if (line[0] == '#' || line[0] == '\n' || linlen <= 0)
	continue;
      if (sscanf (line, " IACAMODULE %ms", &name))
	{
	  iaca_debug ("module '%s'", name);
	  GModule *mod = g_module_open (name, 0);
	  if (!mod)
	    iaca_error ("failed to load module '%s' - %s",
			name, g_module_error ());
	  g_hash_table_insert (iaca_module_htab, g_strdup (name), mod);
	}
      else if (sscanf (line, " IACADATA %ms", &name))
	{
	  char *datapath = 0;
	  iaca_debug ("data '%s'", name);
	  datapath = g_build_filename (dirpath,
				       g_strconcat (name, ".json", NULL),
				       NULL);
	  iaca_debug ("datapath '%s'", datapath);
	  if (!g_file_test (datapath, G_FILE_TEST_EXISTS))
	    iaca_error ("data file %s does not exist", datapath);
	  iaca_load_data (&ld, datapath);
	}
    }
}
