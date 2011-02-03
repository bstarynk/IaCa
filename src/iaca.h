#ifndef IACA_INCLUDED
#define IACA_INCLUDED

/****
  © 2011 Basile Starynkevitch 
    this file iaca.h is part of IaCa

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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

/// standard includes
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/// system includes
#include <dlfcn.h>
#include <sys/types.h>


// JANSSON include for JSON - see http://www.digip.org/jansson/
#include <jansson.h>

// GTK include for graphical interface
#include <gtk/gtk.h>

// Boehm's garbage collector
#include <gc/gc.h>

/*****************************************************************/
/* error reporting */

/* fatal error: */
#define iaca_error(Fmt,...) do {		\
g_error("%s:%d <%s> " Fmt "\n",			\
	basename(__FILE__), __LINE__, __func__, \
##__VA_ARGS__); } while(0)

/* critical error: */
#define iaca_critical(Fmt,...) do {			\
    g_critical ("%s:%d <%s> " Fmt "\n",			\
		basename(__FILE__), __LINE__, __func__,	\
		##__VA_ARGS__); } while(0)

/* warning: */
#define iaca_warning(Fmt,...) do {			\
    g_warning ("%s:%d <%s> " Fmt "\n",			\
		basename(__FILE__), __LINE__, __func__,	\
		##__VA_ARGS__); } while(0)

/* information message: */
#define iaca_message(Fmt,...) do {			\
    g_message ("%s:%d <%s> " Fmt "\n",			\
		basename(__FILE__), __LINE__, __func__,	\
		##__VA_ARGS__); } while(0)

/* debug message: */
#define iaca_debug_at(Fil,Lin,Fmt,...) do {	\
    g_debug ("%s:%d <%s> " Fmt,			\
	     basename(Fil), Lin, __func__,	\
	     ##__VA_ARGS__); } while(0)
#define iaca_debug(Fmt,...) \
 iaca_debug_at(__FILE__,__LINE__,Fmt,##__VA_ARGS__)

/*****************************************************************/
// allocate a garbage collected data and clear it. The data might
// later contain pointers and other things to be followed by the
// garbage collector.
static inline void *iaca_alloc_data (size_t sz);

// for optimization, allocate a garbage collected data and clear
// it. However, we promise that the data will never contain any
// pointer. Boehm's garbage collector will handle it more efficiently.
static inline void *iaca_alloc_atomic (size_t sz);

// our value kinds
enum iacavaluekind_en
{
  IACAV__NONE = 0,		/* zero, never used */

  IACAV_INTEGER,		/* boxed integer value */
  IACAV_STRING,			/* UTF8 string value */
  IACAV_NODE,			/* immutable node value */
  IACAV_SET,			/* immutable dichotomized set of items */
  IACAV_ITEM,			/* [shared mutable] item with payload */
};

//// forward declarations
typedef struct iacavalue_st IacaValue;	/// any value above
typedef struct iacainteger_st IacaInteger;	/// boxed integer
typedef struct iacastring_st IacaString;	/// boxed string
typedef struct iacanode_st IacaNode;	/// node
typedef struct iacaset_st IacaSet;	/// immutable set
typedef struct iacaitem_st IacaItem;	/// shared mutable item with payload

/// every value starts with a discriminating kind
struct iacavalue_st
{
  unsigned v_kind;		/* the kind never changes and is non-zero! */
};

/******************** BOXED INTEGERS *************/
/*** JSON:
     { "kd" : "intv" , "int" : <number> } 
***/
struct iacainteger_st
{
  unsigned v_kind;		/* always IACAV_INTEGER */
  long v_num;			/* the immutable value */
};

// allocator
static inline IacaInteger *iaca_integer_make (long l);

#define iacav_integer_make(L) ((IacaValue*)iaca_integer_make((L)))

// safe accessor with default
static inline long iaca_integer_val_def (IacaValue *v, long def);

// safe accessor or 0
#define iaca_integer_val(V) iaca_integer_val_def((V),0L)

// safe casting to IacaInteger
static inline IacaInteger *iacac_integer (IacaValue *v);

/******************** STRING VALUES *************/
/*** JSON:
     { "kd" : "strv" , "str" : <string> } 
***/

struct iacastring_st
{
  unsigned v_kind;		/* always IACAV_STRING */
  // the array of valid UTF8 char; last is null
  gchar v_str[];
};

// allocator of a string; return null if invalid
static inline IacaString *iaca_string_make (const gchar *s);

#define iacav_string_make(L) ((IacaString*)iaca_integer_make((L)))

// safe accessor with default
static inline const gchar *iaca_string_val_def (IacaValue *v,
						const gchar *def);

// safe accessor or null string
#define iaca_string_val(V) iaca_string_val_def((V),NULL)

// safe accessor or empty string
#define iaca_string_valempty(V) iaca_string_val_def((V),"")

// safe cast to IacaString
static inline IacaString *iacac_string (IacaValue *v);


/***************** NODE VALUES ****************/
/*** JSON:
     { "kd" : "nodv" , "conid" : <int-id> , "sons" : [ <son-values...> ] } 
***/

/* Nodes are immutable, and have a non-null connector item and sons */
struct iacanode_st
{
  unsigned v_kind;		/* always IACAV_KIND */
  unsigned v_arity;		/* node arity, could even be a
				   million, but much more is perhaps
				   unreasonable */
#define IACA_ARITY_MAX (1<<22)
  IacaItem *v_conn;
  IacaValue *v_sons[];		/* size is v_arity */
};

// allocator of a node with its array of sons; return null if no
// connective
extern IacaNode *iaca_node_make (IacaValue *conn, IacaValue *sontab[],
				 unsigned arity);
#define iacav_node_make(Con,Sons,Ari) \
  ((IacaValue*)iaca_node_make((Con),(Sons),(Ari)))
// variadic allocator of a node with its sons, null terminated
extern IacaNode *iaca_node_makevarf (IacaValue *conn, ...)
  __attribute__ ((sentinel));
#define iaca_node_makevar(Conn,...) \
  iaca_node_makevarf(Conn,##__VA_ARGS__,(IacaValue*)0)

// safe accessors
// get the connective or a default
static inline IacaValue *iaca_node_conn_def (IacaValue *v, IacaValue *def);
#define iaca_node_conn(V) iaca_node_conn_def(v,(IacaValue*)0)

// get the arity or a default; gives a signed integer
static inline int iaca_node_arity_def (IacaValue *v, int def);

#define iaca_node_arity(V) iaca_node_arity_def((V),0)
#define iaca_node_aritym1(V) iaca_node_arity_def((V),-1)

// get the n-th son or a default. If n is negative, count from end.
static inline IacaValue *iaca_node_son_def (IacaValue *v, int n,
					    IacaValue *def);

#define iaca_node_son(V,N) iaca_node_son_def((V),(N),NULL)

// safe cast to IacaNode
static inline IacaNode *iacac_node (IacaValue *v);

/***************** SET VALUES ****************/
/*** JSON:
     { "kd" : "setv" ,  "elem" : [ <element-ids...> ] } 
***/


/* Sets are immutable, and have a dichotomical array of items, so
   membership test is logarithmic */
struct iacaset_st
{
  unsigned v_kind;		/* always IACAV_SET */
  unsigned v_cardinal;		/* the cardinal of the set */
#define IACA_CARDINAL_MAX (1<<24)
  IacaItem *v_elements[];
};

// allocator of a set with a parent set and an  array of item elements.
extern IacaNode *iaca_set_make (IacaValue *parentset, IacaValue *elemtab[],
				unsigned arity);
// variadic allocator of a set with its parent and elements, null terminated
extern IacaNode *iaca_set_makevarf (IacaValue *parentset, ...)
  __attribute__ ((sentinel));
#define iaca_set_makevar(Par,...) \
  iaca_node_makevarf(Par,##__VA_ARGS__,(IacaValue*)0)
#define iaca_set_makenewvar(...) \
  iaca_node_makevarf((IacaValue*)0,##__VA_ARGS__,(IacaValue*)0)

static inline int iaca_set_cardinal_def (IacaValue *v, int def);

#define iaca_set_cardinal(V) iaca_set_cardinal_def((V),0)
#define iaca_set_cardinalm1(V) iaca_set_cardinal_def((V),-1)

/* membership test is logarithmic; implemented below! */
static inline bool iaca_set_contains (IacaValue *vset, IacaValue *vitem);

/* given a set return its first element or null */
static inline IacaValue *iaca_set_first_element (IacaValue *vset);
/* given a set return the lowest element after a given one or null */
static inline IacaValue *iaca_set_after_element (IacaValue *vset,
						 IacaValue *velem);

/***************** SHARED ITEM VALUES ****************/
/****
  JSON representation. We serialize separately item shared references
  (= pointers to IacaItem) and item contents (=the data inside a
  IacaItem). 
  Item references are serialized as
  { "kd" : "itrv" ,  "id" : <id-number> } 

  Item contents are serialized like
  { "item" : <id-number> ,
    "attr" : [ <item-attribute...> ] ,
    "payload" : <item-payload> 
  }

***/


/* Sets are immutable, and have a dichotomical array of items, so

****/
/* item payload */
enum iacapayloadkind_en
{
  IACAPAYLOAD__NONE,
  IACAPAYLOAD_VECTOR,
  IACAPAYLOAD_DICTIONNARY,
};

/* Item structure; each has a unique 64 bits identifying unique
   integer. On an Intel Q9550 @ 2.83GHz, incrementing a counter in a
   buzy loop takes 2.12ns/iter, that is 77 years to reach 2^60. If
   calling a GC_MALLOC inside a loop, it takes at least 53ns/iter,
   that is 1935 years to reach 2^60. So incrementing a persistent 64
   bits counter seems practical, even on a 10x time faster machine for
   several years. On a more recent AMD Phenom(tm) II X4 955 timings
   are similar. */
struct iacaitem_st
{
  unsigned v_kind;		/* always IACAV_ITEM */
  enum iacapayloadkind_en v_payloadkind;	/* the variable payload kind */
  int64_t v_ident;		/* the immutable unique identifying number */
  struct iacatabattr_st *v_attrtab;	/* the hash table of attribute
					   entries */
  IacaValue *v_itemcontent;	/* the mutable item content */
  struct iacaspace_st *v_space;	/* the item space defines where it is
				   persisted */
  /* anonymous union of pointers for payload. Change simultanously the
     v_payloadkind. */
  union
  {
    void *v_payloadptr;
    struct iacapayloadvector_st *v_payloadvect;
    struct iacapayloaddictionnary_st *v_payloaddict;
  };
};

int64_t iaca_item_last_ident;

/* make an item */
extern IacaItem *iaca_item_make (void);
#define iacav_item_make() ((IacaValue*) iaca_item_make())

struct iacaentryattr_st
{
  IacaItem *en_item;		/* an item, or IACA_EMPTY_SLOT */
#define IACA_EMPTY_SLOT  ((IacaItem*)-1L)
  IacaValue *en_val;
};

struct iacatabattr_st
{
  unsigned at_size;		/* allocated prime size */
  unsigned at_count;		/* used count */
  struct iacaentryattr_st at_entab[];	/* size is at_size */
};

/* in item vitem, physically get the value associate to attribute
   vattr or else default vdef */
static inline IacaValue *iaca_item_attribute_physical_get_def (IacaValue
							       *vitem,
							       IacaValue
							       *vattr,
							       IacaValue
							       *vdef);

#define iaca_item_attribute_physical_get(Vitem,Vattr) \
  iaca_item_attribute_physical_get_def((Vitem),(Vattr),(IacaValue*)0)

/* inside an item vitem put attribute vattr associated to value
   val. Don't do anything if vitem or vattr are not items, or if val
   is null. May grow the table if it was full. */
extern void iaca_item_physical_put (IacaValue *vitem, IacaValue *vattr,
				    IacaValue *val);

/* inside an item vitem remove attribute vattr and return its previous
   value. Don't do anything if vitem or vattr are not items. May
   shrink the table if it was nearly empty. */
extern IacaValue *iaca_item_physical_remove (IacaValue *vitem,
					     IacaValue *vattr);

/* Reorganize an item attribute table and reserve space for xtra
   attributes, perhaps shrinking or growing the table. */
extern void iaca_item_attribute_reorganize (IacaValue *vitem, unsigned xtra);

/* in item vitem, get the first attribute */
static inline IacaValue *iaca_item_first_attribute (IacaValue *vitem);

/* in item vitem, get the attribute following a given attribute, or else null */
static inline IacaValue *iaca_item_next_attribute (IacaValue *vitem,
						   IacaValue *vattr);

/* iterate inside an item attribute; Item and Attr should be local
   variables; Item should not be modified inside the for body. */
#define IACA_FOREACH_ITEM_ATTRIBUTE(Item,Attr)		\
  for(Attr=iaca_item_first_attribute ((Item));		\
      (Attr) != NULL;					\
      Attr = iaca_item_next_attribute((Item),(Attr)))

static inline bool iaca_set_contains (IacaValue *vset, IacaValue *vitem);

static inline IacaValue *iaca_set_first_element (IacaValue *vset);

static inline IacaValue *iaca_set_after_element (IacaValue *vset,
						 IacaValue *velem);

/* iterate inside a set; Set and Elem should be local variables. Set
   should not be modified inside the for body. */
#define IACA_FOREACH_SET_ELEMENT(Set,Elem)		\
  for (Elem=iaca_set_first_element((Set));		\
       (Elem) != NULL;					\
       Elem = iaca_set_after_element((Set),(Elem)))




/*****************************************************************************
 *****************************************************************************
 *****************************************************************************/

static inline void *
iaca_alloc_data (size_t sz)
{
  void *v = GC_MALLOC (sz);
  memset (v, 0, sz);
  return v;
}

static inline void *
iaca_alloc_atomic (size_t sz)
{
  void *v = GC_MALLOC_ATOMIC (sz);
  memset (v, 0, sz);
  return v;
}

static inline IacaInteger *
iaca_integer_make (long l)
{
  IacaInteger *v = iaca_alloc_atomic (sizeof (IacaInteger));
  v->v_kind = IACAV_INTEGER;
  v->v_num = l;
  return v;
}

// safe accessor with default
static inline long
iaca_integer_val_def (IacaValue *v, long def)
{
  if (!v || v->v_kind != IACAV_INTEGER)
    return def;
  return ((IacaInteger *) v)->v_num;
}

// safe casting to IacaInteger
static inline IacaInteger *
iacac_integer (IacaValue *v)
{
  if (!v || v->v_kind != IACAV_INTEGER)
    return NULL;
  return ((IacaInteger *) v);
}

// allocator of a string; return null if invalid
static inline IacaString *
iaca_string_make (const gchar *s)
{
  const gchar *ends = 0;
  // accept null pointer, and ensure argument is valid UTF8
  if (!s)
    s = "";
  if (!g_utf8_validate (s, (gssize) - 1, &ends))
    return 0;
  gsize sz = ends - s;
  IacaString *v =
    iaca_alloc_atomic (sizeof (IacaInteger) + (sz + 1) * sizeof (gchar));
  v->v_kind = IACAV_STRING;
  memcpy (v->v_str, s, sz);
  return v;
}

// safe accessor with default
static inline const gchar *
iaca_string_val_def (IacaValue *v, const gchar *def)
{
  if (!v || v->v_kind != IACAV_STRING)
    return def;
  return ((IacaString *) v)->v_str;
}

// safe cast to IacaString
static inline IacaString *
iacac_string (IacaValue *v)
{
  if (!v || v->v_kind != IACAV_STRING)
    return NULL;
  return ((IacaString *) v);
}

// safe accessors
// get the connective or a default
static inline IacaValue *
iaca_node_conn_def (IacaValue *v, IacaValue *def)
{
  if (!v || v->v_kind != IACAV_NODE)
    return def;
  return (IacaValue *) (((IacaNode *) v)->v_conn);
}

// get the arity or a default; gives a signed integer
static inline int
iaca_node_arity_def (IacaValue *v, int def)
{
  if (!v || v->v_kind != IACAV_NODE)
    return def;
  return ((IacaNode *) v)->v_arity;
}

// get the n-th son or a default. If n is negative, count from end.
static inline IacaValue *
iaca_node_son_def (IacaValue *v, int n, IacaValue *def)
{
  if (!v || v->v_kind != IACAV_NODE)
    return def;
  if (n < 0)
    n += ((IacaNode *) v)->v_arity;
  if (n >= 0 && n < ((IacaNode *) v)->v_arity)
    return ((IacaNode *) v)->v_sons[n];
  return def;
}

// safe cast to IacaNode
static inline IacaNode *
iacac_node (IacaValue *v)
{
  if (!v || v->v_kind != IACAV_NODE)
    return NULL;
  return ((IacaNode *) v);
}

static inline int
iaca_set_cardinal_def (IacaValue *v, int def)
{
  if (!v || v->v_kind != IACAV_SET)
    return def;
  return ((IacaSet *) v)->v_cardinal;
}

/* internal UNSAFE routine returning the index of an attribute inside
   a table, or else -1. Don't call it unless you are sure that tbl is
   a non-null table, and itat is an item! */
static inline int
iaca_attribute_index_unsafe (struct iacatabattr_st *tbl, IacaItem *itat)
{
  unsigned sz = tbl->at_size;
  unsigned h = itat->v_ident % sz;
  for (unsigned ix = h; ix < sz; ix++)
    {
      IacaItem *curit = tbl->at_entab[ix].en_item;
      if (curit == itat)
	return (int) ix;
      else if (curit == IACA_EMPTY_SLOT)	/* emptied slot */
	continue;
      else if (!curit)
	return -1;
    }
  for (unsigned ix = 0; ix < h; ix++)
    {
      IacaItem *curit = tbl->at_entab[ix].en_item;
      if (curit == itat)
	return (int) ix;
      else if (curit == IACA_EMPTY_SLOT)	/* emptied slot */
	continue;
      else if (!curit)
	return -1;
    }
  return -1;
}

static inline IacaValue *
iaca_item_attribute_physical_get_def (IacaValue *vitem, IacaValue *vattr,
				      IacaValue *vdef)
{
  IacaItem *item = 0;
  IacaItem *attr = 0;
  struct iacatabattr_st *tbl = 0;
  int ix = -1;
  if (!vitem || vitem->v_kind != IACAV_ITEM
      || !vattr || vattr->v_kind != IACAV_ITEM)
    return vdef;
  item = (IacaItem *) vitem;
  attr = (IacaItem *) vattr;
  tbl = item->v_attrtab;
  if (!tbl)
    return vdef;
  ix = iaca_attribute_index_unsafe (tbl, attr);
  if (ix < 0)
    return vdef;
  return tbl->at_entab[ix].en_val;
}

/* in item vitem, get the first attribute */
static inline IacaValue *
iaca_item_first_attribute (IacaValue *vitem)
{
  IacaItem *item = 0;
  unsigned sz = 0;
  struct iacatabattr_st *tbl = 0;
  if (!vitem || vitem->v_kind != IACAV_ITEM)
    return NULL;
  item = (IacaItem *) vitem;
  tbl = item->v_attrtab;
  if (!tbl)
    return NULL;
  sz = tbl->at_size;
  for (unsigned ix = 0; ix < sz; ix++)
    {
      IacaItem *curit = tbl->at_entab[ix].en_item;
      if (!curit || curit == IACA_EMPTY_SLOT)	/* emptied slot */
	continue;
      return (IacaValue *) curit;
    }
  return NULL;
}


/* in item vitem, get the attribute following a given attribute, or else null */
static inline IacaValue *
iaca_item_next_attribute (IacaValue *vitem, IacaValue *vattr)
{
  IacaItem *item = 0;
  IacaItem *attr = 0;
  struct iacatabattr_st *tbl = 0;
  int ix = -1;
  unsigned sz = 0;
  if (!vitem || vitem->v_kind != IACAV_ITEM
      || !vattr || vattr->v_kind != IACAV_ITEM)
    return NULL;
  item = (IacaItem *) vitem;
  attr = (IacaItem *) vattr;
  tbl = item->v_attrtab;
  if (!tbl)
    return NULL;
  sz = tbl->at_size;
  ix = iaca_attribute_index_unsafe (tbl, attr);
  if (ix < 0)
    return NULL;
  for (ix = ix; ix < (int) sz; ix++)
    {
      IacaItem *curit = tbl->at_entab[ix].en_item;
      if (!curit || curit == IACA_EMPTY_SLOT)	/* emptied slot */
	continue;
      return (IacaValue *) curit;
    }
}


/* internal UNSAFE routine to get inside a set the index of an item,
   or -1 iff not found. Don't call it, unless you are sure that set is
   a set, item is an item! */
static inline int
iaca_set_index_unsafe (IacaSet *set, IacaItem *item)
{

  int lo = 0;
  int hi = (int) (set->v_cardinal) - 1;
  int md;
  int64_t id = item->v_ident;
  while (lo + 1 < hi)
    {
      IacaItem *curitem = 0;
      md = (lo + hi) / 2;
      curitem = set->v_elements[md];
      if (curitem == item)
	return md;
      if (curitem->v_ident > id)
	hi = md;
      else if (curitem->v_ident < id)
	lo = md;
      else
	g_assert (curitem == item);
    };
  for (md = lo; md <= hi; md++)
    {
      IacaItem *curitem = set->v_elements[md];
      if (curitem == item)
	return md;
    }
  return -1;
}

static inline bool
iaca_set_contains (IacaValue *vset, IacaValue *vitem)
{
  if (!vset || vset->v_kind != IACAV_SET)
    return false;
  if (!vitem || vitem->v_kind != IACAV_ITEM)
    return false;
  return iaca_set_index_unsafe ((IacaSet *) vset, (IacaItem *) vitem) >= 0;
}

static inline IacaValue *
iaca_set_first_element (IacaValue *vset)
{
  IacaSet *set = 0;
  if (!vset || vset->v_kind != IACAV_SET)
    return NULL;
  set = (IacaSet *) vset;
  if (set->v_cardinal > 0)
    return (IacaValue *) set->v_elements[0];
  return NULL;
}

static inline IacaValue *
iaca_set_after_element (IacaValue *vset, IacaValue *velem)
{
  int lo = 0, hi = 0, md = 0, last = 0;
  int64_t id = 0;
  IacaSet *set = 0;
  IacaItem *item = 0;
  if (!vset || vset->v_kind != IACAV_SET)
    return NULL;
  if (!velem || velem->v_kind != IACAV_ITEM)
    return NULL;
  set = (IacaSet *) vset;
  item = (IacaItem *) velem;
  id = item->v_ident;
  lo = 0;
  last = hi = (int) (set->v_cardinal) - 1;
  while (lo + 1 < hi)
    {
      IacaItem *curitem = 0;
      md = (lo + hi) / 2;
      curitem = set->v_elements[md];
      if (curitem->v_ident > id)
	hi = md;
      else if (curitem->v_ident < id)
	lo = md;
    };
  for (md = lo; md <= hi; md++)
    {
      IacaItem *curitem = set->v_elements[md];
      if (curitem->v_ident > id)
	return (IacaValue *) curitem;
    }
  return NULL;
}

#define IACA_MANIFEST_FILE "IaCa_Manifest"

GHashTable *iaca_module_htab;
GHashTable *iaca_data_htab;
void iaca_load (const char *);
#endif /*IACA_INCLUDED */
