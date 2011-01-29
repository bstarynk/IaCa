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


// YAJL include for JSON
#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>

// GTK include for graphical interface
#include <gtk/gtk.h>

// Boehm's garbage collector
#include <gc/gc.h>

// allocate a garbage collected data and clear it. The data might
// later contain pointers and other things to be followed by the
// garbage collector.
static inline void *
iaca_alloc_data (size_t sz)
{
  void *v = GC_MALLOC (sz);
  memset (v, 0, sz);
  return v;
}

// for optimization, allocate a garbage collected data and clear
// it. However, we promise that the data will never contain any
// pointer. Boehm's garbage collector will handle it more efficiently.
static inline void *
iaca_alloc_atomic (size_t sz)
{
  void *v = GC_MALLOC_ATOMIC (sz);
  memset (v, 0, sz);
  return v;
}

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
struct iacainteger_st
{
  unsigned v_kind;		/* always IACAV_INTEGER */
  long v_num;			/* the immutable value */
};

// allocator
static inline IacaInteger *
iaca_integer_make (long l)
{
  IacaInteger *v = iaca_alloc_atomic (sizeof (IacaInteger));
  v->v_kind = IACAV_INTEGER;
  v->v_num = l;
  return v;
}

#define iacav_integer_make(L) ((IacaValue*)iaca_integer_make((L)))

// safe accessor with default
static inline long
iaca_integer_valdef (IacaValue *v, long def)
{
  if (!v || v->v_kind != IACAV_INTEGER)
    return def;
  return ((IacaInteger *) v)->v_num;
}

// safe accessor or 0
#define iaca_integer_val(V) iaca_integer_valdef((V),0L)

// safe casting to IacaInteger
static inline IacaInteger *
iacac_integer (IacaValue *v)
{
  if (!v || v->v_kind != IACAV_INTEGER)
    return NULL;
  return ((IacaInteger *) v);
}

/******************** STRING VALUES *************/
struct iacastring_st
{
  unsigned v_kind;		/* always IACAV_STRING */
  // the array of valid UTF8 char; last is null
  gchar v_str[];
};

// allocator of a string; return null if invalid
static inline IacaString *
iaca_string_make (const gchar * s)
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

#define iacav_string_make(L) ((IacaString*)iaca_integer_make((L)))

// safe accessor with default
static inline const gchar *
iaca_string_valdef (IacaValue *v, const gchar * def)
{
  if (!v || v->v_kind != IACAV_STRING)
    return def;
  return ((IacaString *) v)->v_str;
}

// safe accessor or null string
#define iaca_string_val(V) iaca_string_valdef((V),NULL)

// safe accessor or empty string
#define iaca_string_valempty(V) iaca_string_valdef((V),"")

// safe cast to IacaString
static inline IacaString *
iacac_string (IacaValue *v)
{
  if (!v || v->v_kind != IACAV_STRING)
    return NULL;
  return ((IacaString *) v);
}


/***************** NODE VALUES ****************/

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
// variadic allocator of a node with its sons, null terminated
extern IacaNode *iaca_node_makevarf (IacaValue *conn, ...)
  __attribute__ ((sentinel));
#define iaca_node_makevar(Conn,...) \
  iaca_node_makevarf(Conn,##__VA_ARGS__,(IacaValue*)0)

// safe accessors
// get the connective or a default
static inline IacaValue *
iaca_node_conndef (IacaValue *v, IacaValue *def)
{
  if (!v || v->v_kind != IACAV_NODE)
    return def;
  return (IacaValue *) (((IacaNode *) v)->v_conn);
}

#define iaca_node_conn(V) iaca_node_conndef(v,(IacaValue*)0)
// get the arity or a default; gives a signed integer
static inline int
iaca_node_aritydef (IacaValue *v, int def)
{
  if (!v || v->v_kind != IACAV_NODE)
    return def;
  return ((IacaNode *) v)->v_arity;
}

#define iaca_node_arity(V) iaca_node_aritydef((V),0)
#define iaca_node_aritym1(V) iaca_node_aritydef((V),-1)
// get the n-th son or a default. If n is negative, count from end.
static inline IacaValue *
iaca_node_sondef (IacaValue *v, int n, IacaValue *def)
{
  if (!v || v->v_kind != IACAV_NODE)
    return def;
  if (n < 0)
    n += ((IacaNode *) v)->v_arity;
  if (n >= 0 && n < ((IacaNode *) v)->v_arity)
    return ((IacaNode *) v)->v_sons[n];
  return def;
}

#define iaca_node_son(V,N) iaca_node_sondef((V),(N),NULL)

// safe cast to IacaNode
static inline IacaNode *
iacac_node (IacaValue *v)
{
  if (!v || v->v_kind != IACAV_NODE)
    return NULL;
  return ((IacaNode *) v);
}

/***************** SET VALUES ****************/

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

static inline int
iaca_set_cardinaldef (IacaValue *v, int def)
{
  if (!v || v->v_kind != IACAV_SET)
    return def;
  return ((IacaSet *) v)->v_cardinal;
}

#define iaca_set_cardinal(V) iaca_set_cardinaldef((V),0)
#define iaca_set_cardinalm1(V) iaca_set_cardinaldef((V),-1)

/* membership test is logarithmic; implemented below! */
static inline bool iaca_set_contains (IacaValue *vset, IacaValue *vitem);

/***************** SHARED ITEM VALUES ****************/
struct iacaitem_st
{
  unsigned v_kind;		/* always IACAV_ITEM */
  unsigned v_nbattr;		/* the number of attributes */
  int64_t v_ident;		/* the unique identifying number */
};


/** implementation of set membership **/

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

#endif /*IACA_INCLUDED */
