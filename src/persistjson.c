
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

struct iacaloader_st
{
  /* Glib hash table asociating item identifiers to loaded item, actually
     the key is a IacaItem structure ... */
  GHashTable *ld_itemhtab;
  /* JANSSON root Json object for a given data file. */
  json_t *ld_root;
  /* to setjmp on error */
  jmp_buf ld_errjmp;
  /* the associated error message GC_strdup-ed */
  const char *ld_errstr;
  /* the originating error line */
  int ld_errline;
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
iaca_json_to_value (struct iacaloader_st *ld, const json_t * js)
{
  IacaValue *res = 0;
  int ty = 0;
  if (!ld)
    iaca_error ("null ld");
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
      }
      break;
    default:
      iaca_json_error_printf (ld, "unexepected json type #%d", ty);
    }
  return res;
}

static void
iaca_load_data (struct iacaloader_st *ld, const char *datapath,
		const char *spacename)
{
  const char *verstr = 0;
  json_error_t jerr;
  memset (&jerr, 0, sizeof (jerr));
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

#warning incomplete iaca_load_data
  /* free the JSON object */
  json_decref (ld->ld_root);
  ld->ld_root = 0;
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
  module = g_hash_table_lookup (iaca_module_htab, modname);
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
	("failed to load module %s in src/ or module/ of %s: %s",
	 modname, dirpath, g_module_error ());
      const char *err = GC_STRDUP (msg);
      g_free (msg);
      return err;
    }
  // the module is resident, we never really g_module_close it!
  g_module_make_resident (module);
  g_hash_table_insert (iaca_module_htab, (gpointer) modname,
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
