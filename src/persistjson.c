
/****
  © 2011 Basile Starynkevitch 
    this file persistjson.c is part of IaCa 
    [persisting data in JSON format thru JANSSON]

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

#define IACA_JSON_VERSION "2011A"

#define IACA_LOADER_MAGIC 0x347b938b	/*880513931 */
struct iacaloader_st
{
  unsigned ld_magic;		/* always IACA_LOADER_MAGIC */
  /* Glib hash table asociating item identifiers to loaded item, actually
     the key is a IacaItem structure ... */
  GHashTable *ld_itemhtab;
  /* JANSSON root Json object for a given data file. */
  json_t *ld_root;
  /* to setjmp on error */
  jmp_buf ld_errjmp;
  /* the associated error message GC_strdup-ed */
  const char *ld_errstr;
  /* the originating error line as given by __LINE__ here */
  int ld_errline;
  /* current dataspace */
  struct iacadataspace_st *ld_dataspace;
};



#define IACA_DUMPER_MAGIC 0x3e4fde5d	/*1045421661 */
struct iacadumper_st
{
  unsigned du_magic;		/* always IACA_DUMPER_MAGIC */
  /* the queue of items to be scanned */
  GQueue *du_scanqueue;
  /* the hash table of already scanned items */
  GHashTable *du_itemhtab;
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
  itm->v_dataspace = ld->ld_dataspace;
  if (iaca.ia_item_last_ident < id)
    iaca.ia_item_last_ident = id;
  itm->v_attrtab = NULL;
  itm->v_payloadkind = IACAPAYLOAD__NONE;
  itm->v_payloadptr = NULL;
  itm->v_itemcontent = NULL;
  g_hash_table_insert (ld->ld_itemhtab, itm, itm);
  return itm;
}

static void iaca_json_error_printf_at (int ln, struct iacaloader_st *ld,
				       const char *fmt, ...)
  __attribute__ ((format (printf, 3, 4), noreturn));
#define iaca_json_error_printf(Ld,Fmt,...) \
  iaca_json_error_printf_at(__LINE__,(Ld),(Fmt),##__VA_ARGS__)
void
iaca_json_error_printf_at (int lin, struct iacaloader_st *ld, const char *fmt,
			   ...)
{
  const char *msg = 0;
  va_list args;
  g_assert (ld && ld->ld_magic == IACA_LOADER_MAGIC);
  va_start (args, fmt);
  msg = g_strdup_vprintf (fmt, args);
  va_end (args);
  ld->ld_errstr = GC_STRDUP (msg);
  g_free ((gpointer) msg);
  msg = 0;
  ld->ld_errline = lin;
  longjmp (ld->ld_errjmp, 1);
}


IacaValue *
iaca_json_to_value (struct iacaloader_st *ld, const json_t *js)
{
  IacaValue *res = 0;
  int ty = 0;
  if (!ld)
    iaca_error ("null ld");
  g_assert (ld && ld->ld_magic == IACA_LOADER_MAGIC);
  if (!js)
    iaca_json_error_printf (ld, "null json pointer");
  ty = json_typeof (js);
  switch (ty)
    {
    case JSON_NULL:
      return NULL;
    case JSON_INTEGER:
      return iacav_integer_make (json_integer_value (js));
    case JSON_STRING:
      return iacav_string_make (json_string_value (js));
    case JSON_OBJECT:
      {
	const char *kdstr = json_string_value (json_object_get (js, "kd"));
	if (!kdstr)
	  iaca_json_error_printf (ld, "missing 'kd' in object");
	else if (!strcmp (kdstr, "strv"))
	  {
	    const char *s = json_string_value (json_object_get (js, "str"));
	    if (s)
	      return iacav_string_make (s);
	    iaca_json_error_printf (ld, "missing 'str' in object for string");
	  }
	else if (!strcmp (kdstr, "intv"))
	  {
	    json_t *intjs = json_object_get (js, "int");
	    if (json_is_integer (intjs))
	      return iacav_integer_make (json_integer_value (intjs));
	    iaca_json_error_printf (ld,
				    "missing 'int' in object for integer");
	  }
	else if (!strcmp (kdstr, "nodv"))
	  {
	    long long conid =
	      json_integer_value (json_object_get (js, "conid"));
	    IacaItem *conitm = 0;
	    json_t *sonjs = 0;
	    int arity = 0;
	    IacaNode *nd = 0;
	    if (conid <= 0)
	      iaca_json_error_printf (ld,
				      "invalid or missing 'conid' in object for node");
	    sonjs = json_object_get (js, "sons");
	    if (!json_is_array (sonjs))
	      iaca_json_error_printf (ld, "bad 'sons' in object for node");
	    arity = json_array_size (sonjs);
	    conitm = iaca_retrieve_loaded_item (ld, conid);
	    nd = iaca_node_make ((IacaValue *) conitm, 0, arity);
	    for (int i = 0; i < arity; i++)
	      nd->v_sons[i] =
		iaca_json_to_value (ld, json_array_get (sonjs, i));
	    return (IacaValue *) nd;
	  }
	else if (!strcmp (kdstr, "setv"))
	  {
	    json_t *elemjs = 0;
	    int card = 0;
	    IacaValue **elemtab = 0;
	    elemjs = json_object_get (js, "elemids");
	    if (!json_is_array (elemjs))
	      iaca_json_error_printf (ld, "bad 'elemids' in object for set");
	    card = json_array_size (elemjs);
	    elemtab = iaca_alloc_data (sizeof (IacaItem *) * card);
	    for (int i = 0; i < card; i++)
	      {
		json_t *curelemjs = json_array_get (elemjs, i);
		long long curelemid = 0;
		elemtab[i] = NULL;
		if (!json_is_integer (curelemjs))
		  iaca_json_error_printf (ld,
					  "element #%d in object for set not integer",
					  i);
		curelemid = json_integer_value (curelemjs);
		elemtab[i] =
		  (IacaValue *) iaca_retrieve_loaded_item (ld, curelemid);
	      }
	    res = (IacaValue *) iaca_set_make (NULL, elemtab, card);
	    GC_free (elemtab);
	    return res;
	  }
	else if (!strcmp (kdstr, "itrv"))
	  {
	    long long id = json_integer_value (json_object_get (js, "id"));
	    if (id > 0)
	      return (IacaValue *) iaca_retrieve_loaded_item (ld, id);
	    else
	      iaca_json_error_printf (ld,
				      "bad or missing id %lld in object for item reference",
				      id);
	  }
	else
	  iaca_json_error_printf (ld, "bad kind string %s in object", kdstr);
      }
      break;
    default:
      iaca_json_error_printf (ld, "unexepected json type #%d", ty);
    }
  return res;
}


static void
iaca_load_item_pay_load (struct iacaloader_st *ld, IacaItem *itm, json_t *js)
{
  g_assert (ld && ld->ld_magic == IACA_LOADER_MAGIC);
  g_assert (itm && itm->v_kind == IACAV_ITEM);
  if (!js)
    iaca_json_error_printf (ld, "no item #%lld payload",
			    (long long) itm->v_ident);
  if (json_is_null (js))
    {
      itm->v_payloadkind = IACAPAYLOAD__NONE;
      itm->v_payloadptr = NULL;
      return;
    }
  else if (json_is_object (js))
    {
      const char *kdstr =
	json_string_value (json_object_get (js, "payloadkind"));
      if (!strcmp (kdstr, "vector"))
	{
	  json_t *jsarr = json_object_get (js, "payloadvector");
	  int sz = 0;
	  if (!json_is_array (jsarr))
	    iaca_json_error_printf (ld, "bad item #%lld vector payload",
				    (long long) itm->v_ident);
	  sz = json_array_size (jsarr);
	  iaca_item_pay_load_resize_vector (itm, sz);
	  for (int ix = 0; ix < sz; ix++)
	    iaca_item_pay_load_append_vector (itm,
					      iaca_json_to_value (ld,
								  json_array_get
								  (jsarr,
								   ix)));
	}
      else if (!strcmp (kdstr, "buffer"))
	{
	  int ln = json_integer_value (json_object_get (js, "payloadbuflen"));
	  json_t *jsarr = json_object_get (js, "payloadbuffer");
	  int sz = 0;
	  iaca_item_pay_load_reserve_buffer (itm, ln + 2);
	  if (!json_is_array (jsarr))
	    iaca_json_error_printf (ld, "bad item #%lld buffer payload",
				    (long long) itm->v_ident);
	  sz = json_array_size (jsarr);
	  for (int ix = 0; ix < sz; ix++)
	    {
	      if (ix > 0)
		iaca_item_pay_load_append_buffer (itm, "\n");
	      iaca_item_pay_load_append_buffer (itm,
						json_string_value
						(json_array_get (jsarr, ix)));
	    }
	}
    }
#warning iaca_load_item_pay_load incomplete
}



static void
iaca_load_item_content (struct iacaloader_st *ld, json_t *js)
{
  long long id = 0;
  IacaItem *itm = 0;
  json_t *jsattrs = 0;
  int nbattrs = 0;
  g_assert (ld && ld->ld_magic == IACA_LOADER_MAGIC);
  if (!json_is_object (js))
    iaca_json_error_printf (ld, "expecting an object for item content");
  id = json_integer_value (json_object_get (js, "item"));
  if (id <= 0)
    iaca_json_error_printf (ld, "invalid id %lld for loaded item content",
			    id);
  itm = iaca_retrieve_loaded_item (ld, id);
  if (itm->v_dataspace)
    iaca_json_error_printf
      (ld, "loaded item #%lld has dataspace %s",
       id,
       iaca_string_val_def ((IacaValue *) itm->v_dataspace->dsp_name, "??"));
  itm->v_dataspace = ld->ld_dataspace;
  jsattrs = json_object_get (js, "itemattrs");
  if (!json_is_array (jsattrs))
    iaca_json_error_printf (ld, "loaded item #%lld without itemattrs", id);
  nbattrs = json_array_size (jsattrs);
  for (int ix = 0; ix < nbattrs; ix++)
    {
      json_t *jscurat = json_array_get (jsattrs, ix);
      long long atid = 0;
      IacaValue *val = 0;
      IacaItem *atitm = 0;
      if (!json_is_object (jscurat))
	iaca_json_error_printf (ld, "attribute entry is not a Json object");
      atid = json_integer_value (json_object_get (jscurat, "atid"));
      if (atid <= 0)
	iaca_json_error_printf (ld,
				"bad attribute id #%lld in item #%lld content",
				atid, id);
      val = iaca_json_to_value (ld, json_object_get (jscurat, "val"));
      if (!val)
	continue;
      atitm = iaca_retrieve_loaded_item (ld, atid);
      iaca_item_physical_put ((IacaValue *) itm, (IacaValue *) atitm, val);
    }
  iaca_load_item_pay_load (ld, itm, json_object_get (js, "itempayload"));
}

static void
iaca_load_data (struct iacaloader_st *ld, const char *datapath,
		const char *spacename)
{
  const char *verstr = 0;
  json_error_t jerr;
  json_t *jsitarr = 0;
  size_t nbit = 0;
  memset (&jerr, 0, sizeof (jerr));
  g_assert (ld && ld->ld_magic == IACA_LOADER_MAGIC);
  ld->ld_dataspace = iaca_dataspace (spacename);
  ld->ld_root = json_load_file (datapath, 0, &jerr);
  if (!ld->ld_root)
    iaca_error ("failed to load data file %s: JSON error line %d: %s",
		datapath, jerr.line, jerr.text);
  iaca_debug ("loaded root %p from data %s", ld->ld_root, datapath);
  if (!json_is_object (ld->ld_root))
    iaca_error ("JSON root in %s not an object", datapath);
  verstr = json_string_value (json_object_get (ld->ld_root, "iacaversion"));
  if (!verstr)
    iaca_error ("JSON root without version in data file %s", datapath);
  if (strcmp (verstr, IACA_JSON_VERSION))
    iaca_error
      ("JSON root with iacaversion %s but expecting %s in data file %s",
       verstr, IACA_JSON_VERSION, datapath);
  jsitarr = json_object_get (ld->ld_root, "itemcont");
  if (!json_is_array (jsitarr))
    iaca_error ("JSON root without itemcont in data file %s", datapath);
  nbit = json_array_size (jsitarr);
  for (int ix = 0; ix < nbit; ix++)
    {
      json_t *jscurit = json_array_get (jsitarr, ix);
      iaca_load_item_content (ld, jscurit);
    }
  /* free the JSON object */
  json_decref (ld->ld_root);
  ld->ld_root = 0;
  ld->ld_dataspace = NULL;
}




const char *
iaca_load_module (const char *dirpath, const char *modname)
{
  gchar *modulepath = 0;
  gchar *moduledirpath = 0;
  GModule *module = 0;
  iaca_debug ("dirpath '%s' modname '%s'", dirpath, modname);
  if (!dirpath || !dirpath[0])
    return GC_STRDUP ("emptyy dirpath to load module");
  if (!modname || !modname[0])
    return GC_STRDUP ("empty modname to load module");
  // check that modules are supported
  if (!g_module_supported ())
    return GC_STRDUP ("modules are not supported");
  // check validity of dirpath
  if (!g_file_test (dirpath, G_FILE_TEST_IS_DIR))
    {
      char *msg =
	g_strdup_printf ("when loading module dirpath %s is not a directory",
			 dirpath);
      const char *err = GC_STRDUP (msg);
      g_free (msg);
      return err;
    }
  // check validity of modname
  for (const char *pc = modname; *pc; pc++)
    if (!g_ascii_isalnum (*pc) && *pc != '_')
      {
	char *msg =
	  g_strdup_printf
	  ("when loading module invalid character in module name %s",
	   modname);
	const char *err = GC_STRDUP (msg);
	g_free (msg);
	return err;
      };
  //
  module = g_hash_table_lookup (iaca.ia_module_htab, modname);
  if (module)
    {
      char *msg = g_strdup_printf ("module %s already loaded as %s", modname,
				   g_module_name (module));
      const char *err = GC_STRDUP (msg);
      g_free (msg);
      return err;
    }
  // look for the module in the src/ subdirectory
  if (!module)
    {				/* always true here */
      moduledirpath = g_build_filename (dirpath, "src", NULL);
      if (g_file_test (moduledirpath, G_FILE_TEST_IS_DIR))
	{
	  modulepath = g_module_build_path (moduledirpath, modname);
	  module = g_module_open (modulepath, 0);
	}
      g_free (moduledirpath);
      moduledirpath = 0;
    }
  // look for the module in the module/ subdirectory
  if (!module)
    {				/* true if module not found */
      moduledirpath = g_build_filename (dirpath, "module", NULL);
      if (g_file_test (moduledirpath, G_FILE_TEST_IS_DIR))
	{
	  modulepath = g_module_build_path (moduledirpath, modname);
	  module = g_module_open (modulepath, 0);
	}
      g_free (moduledirpath);
      moduledirpath = 0;
    }
  if (!module)
    {
      char *msg
	=
	g_strdup_printf
	("failed to load module %s in src/ or module/ of %s : %s",
	 modname, dirpath, g_module_error ());
      const char *err = GC_STRDUP (msg);
      g_free (msg);
      return err;
    }
  // the module is resident, we never really g_module_close it!
  g_module_make_resident (module);
  g_hash_table_insert (iaca.ia_module_htab, (gpointer) modname,
		       (gpointer) module);
  return NULL;
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
  ld.ld_magic = IACA_LOADER_MAGIC;
  ld.ld_itemhtab = g_hash_table_new (iaca_item_ghash, iaca_item_gheq);
  fil = fopen (manipath, "r");
  if (!fil)
    iaca_error ("failed to open manifest file %s - %m", manipath);
  siz = 80;
  line = calloc (siz, 1);
  if (!iaca.ia_module_htab)
    iaca.ia_module_htab = g_hash_table_new (g_str_hash, g_str_equal);
  if (!iaca.ia_dataspace_htab)
    iaca.ia_dataspace_htab = g_hash_table_new (g_str_hash, g_str_equal);
  while (!feof (fil))
    {
      ssize_t linlen = getline (&line, &siz, fil);
      char *name = 0;
      // skip comment or empty line in manifest
      if (line[0] == '#' || line[0] == '\n' || linlen <= 0)
	continue;
      if (sscanf (line, " IACAMODULE %ms", &name))
	{
	  const char *errstr = 0;
	  iaca_debug ("module '%s'", name);
	  errstr = iaca_load_module (dirpath, name);
	  if (errstr)
	    iaca_error ("failed to load module '%s' - %s", name, errstr);
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
	  iaca_load_data (&ld, datapath, name);
	}
    }
}





/*****************************************************************/

// routine to queue an item to be scanned for dumping; return true if
// the item is transient and should be ignored
bool
iaca_dump_queue_item (struct iacadumper_st *du, IacaItem *itm)
{
  IacaItem *fnditm = 0;
  if (!du || !itm || itm->v_kind != IACAV_ITEM)
    return true;
  g_assert (du->du_magic == IACA_DUMPER_MAGIC);
  g_assert (du->du_scanqueue);
  g_assert (du->du_itemhtab);
  fnditm = g_hash_table_lookup (du->du_itemhtab, itm);
  if (fnditm)
    {				/* item already scanned */
      g_assert (fnditm == itm);
      return false;
    }
  /** we probably should add a hook to add a dataspace if none */
  /* ignore item without dataspace */
  if (!itm->v_dataspace)
    return true;
  g_queue_push_tail (du->du_scanqueue, (gpointer) itm);
  return false;
}


// test if an item is transient, that is should not be dumped!
static inline bool
iaca_dump_item_is_transient (struct iacadumper_st *du, IacaItem *itm)
{
  if (!du || !itm)
    return true;
  g_assert (du->du_magic == IACA_DUMPER_MAGIC);
  g_assert (itm->v_kind == IACAV_ITEM);
  g_assert (du->du_itemhtab);
  if (g_hash_table_lookup (du->du_itemhtab, itm))
    return false;
  return true;
}

// recursive routine to scan a data for dumping
void
iaca_dump_scan_value (struct iacadumper_st *du, IacaValue *val)
{
  if (!du)
    return;
  g_assert (du->du_magic == IACA_DUMPER_MAGIC);
scanagain:
  if (!val)
    return;
  switch (val->v_kind)
    {
    case IACAV_INTEGER:
      return;
    case IACAV_STRING:
      return;
    case IACAV_NODE:
      {
	IacaNode *nod = (IacaNode *) val;
	if (iaca_dump_queue_item (du, nod->v_conn))
	  return;
	for (int i = (int)(nod->v_arity) - 1; i > 0; i--)
	  iaca_dump_scan_value (du, nod->v_sons[i]);
	if (nod->v_arity > 0)
	  {
	    val = nod->v_sons[0];
	    goto scanagain;
	  }
	return;
      }
    case IACAV_SET:
      {
	IacaSet *set = (IacaSet *) val;
	for (int i = 0; i < (int) (set->v_cardinal); i++)
	  (void) iaca_dump_queue_item (du, set->v_elements[i]);
	return;
      }
    case IACAV_ITEM:
      (void) iaca_dump_queue_item (du, (IacaItem *) val);
      return;
    default:
      iaca_error ("unexpected value kind %d", (int) val->v_kind);
    }
}


// recursive routine to build a json value from a Iaca value while
// dumping

json_t *
iaca_dump_value_json (struct iacadumper_st *du, IacaValue *val)
{
  json_t *js = 0;
  if (!du)
    return NULL;
  g_assert (du->du_magic == IACA_DUMPER_MAGIC);
  if (!val)
    return json_null ();
  switch (val->v_kind)
    {
    case IACAV_INTEGER:
      {
	IacaInteger *iv = (IacaInteger *) val;
	js = json_object ();
	json_object_set (js, "kd", json_string ("intv"));
	json_object_set (js, "int", json_integer (iv->v_num));
	return js;
      }
    case IACAV_STRING:
      {
	IacaString *is = (IacaString *) val;
	js = json_object ();
	json_object_set (js, "kd", json_string ("strv"));
	json_object_set (js, "str", json_string (is->v_str));
	return js;
      };
    case IACAV_NODE:
      {
	IacaNode *nod = (IacaNode *) val;
	json_t *jsarrson = NULL;
	if (iaca_dump_item_is_transient (du, nod->v_conn))
	  return json_null ();
	js = json_object ();
	json_object_set (js, "kd", json_string ("nodv"));
	json_object_set (js, "conid", json_integer (nod->v_conn->v_ident));
	jsarrson = json_array ();
	for (int ix = 0; ix < nod->v_arity; ix++)
	  json_array_append_new (jsarrson,
				 iaca_dump_value_json (du, nod->v_sons[ix]));
	json_object_set (js, "sons", jsarrson);
	return js;
      }
    case IACAV_SET:
      {
	IacaSet *set = (IacaSet *) val;
	json_t *jsarrelem = NULL;
	js = json_object ();
	json_object_set (js, "kd", json_string ("setv"));
	jsarrelem = json_array ();
	for (int ix = 0; ix < set->v_cardinal; ix++)
	  {
	    IacaItem *curitm = 0;
	    curitm = set->v_elements[ix];
	    if (iaca_dump_item_is_transient (du, curitm))
	      continue;
	    json_array_append_new (jsarrelem, json_integer (curitm->v_ident));
	  };
	json_object_set (js, "elemids", jsarrelem);
	return js;
      }
    case IACAV_ITEM:
      {
	IacaItem *itm = (IacaItem *) val;
	if (iaca_dump_item_is_transient (du, itm))
	  return json_null ();
	js = json_object ();
	json_object_set (js, "kd", json_string ("itrv"));
	json_object_set (js, "id", json_integer (itm->v_ident));
	return js;
      }
    default:
      iaca_error ("unexepcted value kind %d", (int) val->v_kind);
    }
}

json_t *
iaca_dump_item_pay_load_json (struct iacadumper_st *du, IacaItem *itm)
{
  json_t *js = 0;
  if (!du || !itm)
    return NULL;
  g_assert (du->du_magic == IACA_DUMPER_MAGIC);
  g_assert (itm->v_kind == IACAV_ITEM);
  switch (itm->v_payloadkind)
    {
    case IACAPAYLOAD__NONE:
      return json_null ();
    case IACAPAYLOAD_VECTOR:
      {
	json_t *jsarr = json_array ();
	unsigned ln = iaca_item_pay_load_vector_length (itm);
	for (unsigned ix = 0; ix < ln; ix++)
	  json_array_append_new (jsarr,
				 iaca_dump_value_json (du,
						       iaca_item_pay_load_nth_vector
						       (itm, ix)));
	js = json_object ();
	json_object_set (js, "payloadkind", json_string ("vector"));
	json_object_set (js, "payloadvector", jsarr);
	return js;
      }
    case IACAPAYLOAD_BUFFER:
      {
	json_t *jsarr = json_array ();
	struct iacapayloadbuffer_st *buf = itm->v_payloadbuf;
	char *bstr = buf ? buf->buf_tab : 0;
	unsigned blen = buf ? buf->buf_len : 0;
	char *s = bstr;
	while (s)
	  {
	    char *eol = strchr (s, '\n');
	    if (eol)
	      {
		*eol = 0;
		json_array_append_new (jsarr, json_string (s));
		s = eol + 1;
	      }
	    else
	      {
		json_array_append_new (jsarr, json_string (s));
		s = 0;
		break;
	      }
	  }
	js = json_object ();
	json_object_set (js, "payloadkind", json_string ("buffer"));
	json_object_set (js, "payloadbuflen", json_integer (blen));
	json_object_set (js, "payloadbuffer", jsarr);
	return js;
      }
    case IACAPAYLOAD_DICTIONNARY:
      iaca_error ("unimplemented iaca_dump_item_payload_json");
    }
}

json_t *
iaca_dump_item_content_json (struct iacadumper_st *du, IacaItem *itm)
{
  json_t *js = 0;
  json_t *jsattr = 0;
  if (!du)
    return NULL;
  g_assert (du->du_magic == IACA_DUMPER_MAGIC);
  if (iaca_dump_item_is_transient (du, itm))
    return json_null ();
  js = json_object ();
  json_object_set (js, "item", json_integer (itm->v_ident));
  jsattr = json_array ();
  IACA_FOREACH_ITEM_ATTRIBUTE_LOCAL ((IacaValue *) itm, vitat)
  {
    IacaValue *atval = NULL;
    json_t *jsentry = NULL;
    if (iaca_dump_item_is_transient (du, (IacaItem *) vitat))
      continue;
    atval = iaca_item_attribute_physical_get ((IacaValue *) itm, vitat);
    if (!atval)
      continue;
    if (atval->v_kind == IACAV_ITEM
	&& iaca_dump_item_is_transient (du, (IacaItem *) atval))
      continue;
    jsentry = json_object ();
    json_object_set (jsentry, "atid",
		     json_integer (((IacaItem *) vitat)->v_ident));
    json_object_set (jsentry, "val", iaca_dump_value_json (du, atval));
    json_array_append_new (jsattr, jsentry);
    jsentry = NULL;
  }
  json_object_set (js, "itemattrs", jsattr);
  json_object_set (js, "itempayload", iaca_dump_item_pay_load_json (du, itm));
  return js;
}
