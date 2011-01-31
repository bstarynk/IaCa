
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
enum iacajsonstate_en
{
  IACAJS__NONE,
  IACAJS_FRESHVAL,		/* parsing a fresh IacaValue */
};

struct iacajsonstate_st
{
  enum iacajsonstate_en js_state;
  union
  {
    long js_num;
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

static inline void
iaca_json_value_push (struct iacaloader_st *ld, IacaValue *va)
{
  int ln = 0;
  GPtrArray *sta = 0;
  if (!ld)
    return;
  sta = ld->ld_valstack;
  if (!sta)
    return;
  g_ptr_array_add (sta, va);
}

static inline void
iaca_json_value_pop (struct iacaloader_st *ld)
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
  g_ptr_array_remove_index (sta, ln - 1);
}

static inline void
iaca_json_state_pop (struct iacaloader_st *ld)
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
  g_array_remove_index (sta, ln - 1);
}

/* various YAJL handlers for loading JSON */

#warning unimplemented YAJL handlers return 0
static int
iacaloadyajl_null (void *ctx)
{
  struct iacaloader_st *ld = ctx;
  struct iacajsonstate_st *topst = iaca_json_top_state (ld);
  if (!topst)
    return 0;
  if (topst->js_state == IACAJS_FRESHVAL)
    {
      iaca_json_state_pop (ld);
      iaca_json_value_push (ld, NULL);
      return 1;
    }
  return 0;
}

static int
iacaloadyajl_boolean (void *ctx, int b)
{
  struct iacaloader_st *ld = ctx;
  return 0;
}

static int
iacaloadyajl_number (void *ctx, const char *s, unsigned l)
{
  struct iacaloader_st *ld = ctx;
  return 0;
}


static int
iacaloadyajl_string (void *ctx, const unsigned char *str, unsigned slen)
{
  struct iacaloader_st *ld = ctx;
  return 0;
}

static int
iacaloadyajl_map_key (void *ctx, const unsigned char *str, unsigned int slen)
{
  struct iacaloader_st *ld = ctx;
  return 0;
}


static int
iacaloadyajl_start_map (void *ctx)
{
  struct iacaloader_st *ld = ctx;
  return 0;
}

static int
iacaloadyajl_end_map (void *ctx)
{
  struct iacaloader_st *ld = ctx;
  return 0;
}


static int
iacaloadyajl_start_array (void *ctx)
{
  struct iacaloader_st *ld = ctx;
  return 0;
}

static int
iacaloadyajl_end_array (void *ctx)
{
  struct iacaloader_st *ld = ctx;
  return 0;
}



static yajl_callbacks lacaloadyajl_callbacks = {
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
iaca_load (const char *dirpath)
{
  gchar *manipath = 0;
  struct iacaloader_st ld = { 0 };
  if (!dirpath || !dirpath[0])
    dirpath = ".";
  manipath = g_build_filename (dirpath, IACA_MANIFEST_FILE, NULL);
  memset (&ld, 0, sizeof (ld));
  ld.ld_itemhtab = g_hash_table_new (iaca_item_ghash, iaca_item_gheq);
}
