
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
#define IACAJS_STATES_LIST()           \
  IACAJS_STATE(IACAJS__NONE),          \
  IACAJS_STATE(IACAJS_FRESHVAL),       \
  IACAJS_STATE(IACAJS_WAITVALKIND),    \
  IACAJS_STATE(IACAJS_WAITITEMID),     \
  IACAJS_STATE(IACAJS_WAITNODECONN),   \
  IACAJS_STATE(IACAJS_WAITNODESONS),   \
  IACAJS_STATE(IACAJS_WAITINTVAL),     \
  IACAJS_STATE(IACAJS_DONEVAL),        \
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
  /* Glib array, stack of IacaValue* pointers */
  GPtrArray *ld_valstack;
  /* Glib array, stack of struct iacajsonstate_st */
  GArray *ld_statestack;
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
iaca_json_top_state (struct iacaloader_st *ld)
{
  int ln = 0;
  GArray *sta = 0;
  if (!ld)
    return NULL;
  sta = ld->ld_statestack;
  if (!sta)
    return NULL;
  ln = sta->len;
  if (ln <= 0)
    return NULL;
  return &g_array_index (sta, struct iacajsonstate_st, ln - 1);
}

static inline IacaValue *
iaca_json_top_value (struct iacaloader_st *ld)
{
  int ln = 0;
  GPtrArray *sta = 0;
  if (!ld)
    return NULL;
  sta = ld->ld_valstack;
  if (!sta)
    return NULL;
  ln = sta->len;
  if (ln <= 0)
    return NULL;
  return (IacaValue *) g_ptr_array_index (sta, ln - 1);
}

#define iaca_json_value_push(Ld,Va) iaca_json_value_push_at(__LINE__,(Ld),(Va))
static inline void
iaca_json_value_push_at (int lin, struct iacaloader_st *ld, IacaValue *va)
{
  GPtrArray *sta = 0;
  if (!ld)
    return;
  sta = ld->ld_valstack;
  if (!sta)
    return;
  iaca_debug_at (__FILE__, lin, "push value [%d] %p", sta->len, va);
  g_ptr_array_add (sta, va);
}

#define iaca_json_value_pop(Ld) iaca_json_value_pop_at(__LINE__,(Ld))
static inline void
iaca_json_value_pop_at (int lin, struct iacaloader_st *ld)
{
  int ln = 0;
  GPtrArray *sta = 0;
  if (!ld)
    return;
  sta = ld->ld_valstack;
  if (!sta)
    return;
  ln = sta->len;
  if (ln <= 0)
    return;
  iaca_debug_at (__FILE__, lin, "pop value [%d]", sta->len);
  g_ptr_array_remove_index (sta, ln - 1);
}

#define iaca_json_state_push(Ld,St) iaca_json_state_push_at(__LINE__,(Ld),(St))
static inline void
iaca_json_state_push_at (int lin, struct iacaloader_st *ld,
			 enum iacajsonstate_en ste)
{
  GArray *sta = 0;
  struct iacajsonstate_st newst = { 0 };
  if (!ld)
    return;
  sta = ld->ld_statestack;
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
  sta = ld->ld_statestack;
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

/* various YAJL handlers for loading JSON */

#warning unimplemented YAJL handlers return 0
static int
iacaloadyajl_null (void *ctx)
{
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  iaca_debug ("start topst %p statedepth %d", topst,
	      ld->ld_statestack ? ld->ld_statestack->len : 0);
  if (!topst)
    return 0;
  iaca_debug ("state #%d [%s]", (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST]);
  switch (topst->js_state)
    {
    case IACAJS_FRESHVAL:
      iaca_json_state_pop (ld);
      iaca_json_value_push (ld, NULL);
      return 1;
    default:
      return 0;
    }
  return 0;
}

static int
iacaloadyajl_boolean (void *ctx, int b)
{
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  iaca_debug ("start topst %p statedepth %d", topst,
	      ld->ld_statestack ? ld->ld_statestack->len : 0);
  if (!topst)
    return 0;
  iaca_debug ("state #%d [%s]", (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST]);
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
	      ld->ld_statestack ? ld->ld_statestack->len : 0);
  if (!topst)
    return 0;
  ll = strtoll (s, &end, 0);
  iaca_debug ("state #%d [%s] ll %lld", (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST], ll);
  switch (topst->js_state)
    {
    case IACAJS_WAITITEMID:
      {
	IacaItem *itm = iaca_retrieve_loaded_item (ld, ll);
	if (!itm)
	  return 0;
	iaca_debug ("item #%lld %p", ll, itm);
	iaca_json_value_push (ld, (IacaValue *) itm);
	topst->js_state = IACAJS_DONEVAL;
	return 1;
      }
    case IACAJS_WAITNODECONN:
      {
	IacaItem *itm = iaca_retrieve_loaded_item (ld, ll);
	if (!itm)
	  return 0;
	if (topst->js_node_conn)
	  return 0;
	iaca_debug ("connective item #%lld %p", ll, itm);
	topst->js_node_conn = itm;
	topst->js_state = IACAJS_WAITNODESONS;
	return 1;
      }
    case IACAJS_WAITINTVAL:
      {
	topst->js_num = (long) ll;
	iaca_debug ("intval %ld", topst->js_num);
	return 1;
      }
    default:
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
	      ld->ld_statestack ? ld->ld_statestack->len : 0);
  if (!topst)
    return 0;
  iaca_debug ("state #%d [%s] str %.*s", (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST], slen, str);
  switch (topst->js_state)
    {
    case IACAJS_WAITVALKIND:
      if (!strncmp ("itrv", (const char *) str, slen))
	{
	  topst->js_state = IACAJS_WAITITEMID;
	  return 1;
	}
      if (!strncmp ("intv", (const char *) str, slen))
	{
	  topst->js_state = IACAJS_WAITINTVAL;
	  return 1;
	}
      else if (!strncmp ("nodv", (const char *) str, slen))
	{
	  topst->js_state = IACAJS_WAITNODECONN;
	  topst->js_node_conn = 0;
	  topst->js_node_sons = 0;
	  topst->js_node_arity = 0;
	  topst->js_node_sizesons = 0;
	  return 1;
	}
      break;
    default:
      return 0;
    }
  return 0;
}

static int
iacaloadyajl_map_key (void *ctx, const unsigned char *str, unsigned int slen)
{
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  iaca_debug ("start topst %p statedepth %d", topst,
	      ld->ld_statestack ? ld->ld_statestack->len : 0);
  if (!topst)
    return 0;
  iaca_debug ("state #%d [%s] key %.*s",
	      (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST], slen, str);
  switch (topst->js_state)
    {
    case IACAJS_WAITVALKIND:
      if (!strncmp ("kd", (const char *) str, slen))
	return 1;
      break;
    case IACAJS_WAITITEMID:
      if (!strncmp ("id", (const char *) str, slen))
	return 1;
    case IACAJS_WAITNODECONN:
      if (!strncmp ("conid", (const char *) str, slen))
	return 1;
      break;
    case IACAJS_WAITNODESONS:
      if (!strncmp ("sons", (const char *) str, slen))
	return 1;
      break;
    case IACAJS_WAITINTVAL:
      if (!strncmp ("int", (const char *) str, slen))
	return 1;
      break;
    default:
      return 0;
    }
  return 0;
}


static int
iacaloadyajl_start_map (void *ctx)
{
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  iaca_debug ("start topst %p statedepth %d", topst,
	      ld->ld_statestack ? ld->ld_statestack->len : 0);
  if (!topst)
    return 0;
  iaca_debug ("state #%d [%s]", (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST]);
  switch (topst->js_state)
    {
    case IACAJS_FRESHVAL:
      iaca_json_state_push (ld, IACAJS_WAITVALKIND);
      return 1;
    case IACAJS_WAITNODESONS:
      iaca_json_state_push (ld, IACAJS_WAITVALKIND);
      return 1;
    default:
      return 0;
    }
  return 0;
}

static int
iacaloadyajl_end_map (void *ctx)
{
  int nbloop = 0;
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  iaca_debug ("start topst %p statedepth %d", topst,
	      ld->ld_statestack ? ld->ld_statestack->len : 0);
  if (!topst)
    return 0;
  while ((topst = iaca_json_top_state (ld)) != 0)
    {
      nbloop++;
      iaca_debug ("state #%d [%s] loop #%d", (int) topst->js_state,
		  iacajs_state_names[topst->js_state % IACAJS__LAST], nbloop);
      switch (topst->js_state)
	{
	case IACAJS_DONEVAL:
	  iaca_json_state_pop (ld);
	  iaca_debug ("done val");
	  return 1;		//continue
	case IACAJS_WAITINTVAL:
	  {
	    IacaValue *val = iacav_integer_make (topst->js_num);
	    iaca_debug ("made int val %ld @%p", topst->js_num, val);
	    iaca_json_value_push (ld, val);
	    iaca_json_state_pop (ld);
	    return 1;		//continue;
	  }
	case IACAJS_WAITNODESONS:
	  {
	    IacaValue *val = 0;
	    val = iaca_json_top_value (ld);
	    iaca_debug ("son #%d %p", topst->js_node_arity, val);
	    iaca_json_value_pop (ld);
	    iaca_json_state_add_node_son (topst, val);
	    return 1;
	  }
	default:
	  return 0;
	}
    }
  if (!topst)
    return 1;
  else
    return 0;
}


static int
iacaloadyajl_start_array (void *ctx)
{
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  iaca_debug ("start topst %p statedepth %d", topst,
	      ld->ld_statestack ? ld->ld_statestack->len : 0);
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
      iaca_json_state_push (ld, IACAJS_FRESHVAL);
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
	      ld->ld_statestack ? ld->ld_statestack->len : 0);
  if (!topst)
    return 0;
  iaca_debug ("state #%d [%s]", (int) topst->js_state,
	      iacajs_state_names[topst->js_state % IACAJS__LAST]);
  switch (topst->js_state)
    {
    case IACAJS_WAITNODESONS:
      {
	IacaValue *newnod =
	  iacav_node_make ((IacaValue *) topst->js_node_conn,
			   topst->js_node_sons, topst->js_node_arity);
	iaca_debug ("newnod %p conn %p arity %d",
		    newnod, topst->js_node_conn, topst->js_node_arity);
	GC_FREE (topst->js_node_sons);
	topst->js_node_sons = 0;
	topst->js_node_arity = topst->js_node_sizesons = 0;
	topst->js_node_conn = 0;
	iaca_json_state_pop (ld);
	iaca_json_value_push (ld, newnod);
	iaca_debug ("ok");
	return 1;
      }
    default:
      return 0;
    }
  return 0;
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
  ld->ld_valstack = g_ptr_array_new ();
  ld->ld_statestack =
    g_array_new (TRUE, TRUE, sizeof (struct iacajsonstate_st));
  linsz = 256;
  line = calloc (linsz, 1);
  iaca_json_state_push (ld, IACAJS_FRESHVAL);
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
  g_ptr_array_free (ld->ld_valstack, TRUE);
  ld->ld_valstack = 0;
  g_array_free (ld->ld_statestack, TRUE);
  ld->ld_statestack = 0;
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
