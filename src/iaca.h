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
#include <setjmp.h>
#include <ctype.h>

/// system includes
#include <dlfcn.h>
#include <sys/types.h>


// JANSSON include for JSON - see http://www.digip.org/jansson/
#include <jansson.h>

// include Glib printf utilities
#include <glib/gprintf.h>
#include <glib/gstdio.h>

// include Glib module facilities
#include <gmodule.h>

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
  if (iaca.ia_debug)				\
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

  IACAV_INTEGER,		/* boxed immutable integer value */
  IACAV_STRING,			/* UTF8 immutable string value */
  IACAV_NODE,			/* immutable copiable node value */
  IACAV_TRANSNODE,		/* transient non-copiable node value */
  IACAV_SET,			/* immutable copiable dichotomized set of items */
  IACAV_ITEM,			/* [shared mutable] item with payload */
  IACAV_GOBJECT,		/* transient Gobject box with value */
};

//// forward declarations
typedef struct iacavalue_st IacaValue;	/// any value above
typedef struct iacainteger_st IacaInteger;	/// boxed integer
typedef struct iacastring_st IacaString;	/// boxed string
typedef struct iacanode_st IacaNode;	/// node
typedef struct iacaset_st IacaSet;	/// immutable set
typedef struct iacaitem_st IacaItem;	/// shared mutable item with payload
typedef struct iacagobject_st IacaGobject;	/* transient boxed GObject */

/// every value starts with a discriminating kind
struct iacavalue_st
{
  unsigned v_kind;		/* the kind never changes and is non-zero! */
};

////////////////////////////////////////////////////////////////
/* the name of tbe IaCa gtk application; should obey to
   g_application_id_is_valid rules */
#define IACA_GTK_APPLICATION_NAME "basile.iaca"

/* the structure describing the entire state of the IaCa system */
extern struct iacastate_st
{
  /* the topmost module for the whole program */
  GModule *ia_progmodule;
  /* hashtable of modules */
  GHashTable *ia_module_htab;
  /* hashtable of data spaces */
  GHashTable *ia_dataspace_htab;
  /* hashtable of iacaclofun_st to speed-up iaca_find_clofun; keys are
     strings */
  GHashTable *ia_clofun_htab;
  /* hashtable associating GObject* to their boxed values if any */
  GHashTable *ia_boxgobject_htab;
  /* last item identifier */
  int64_t ia_item_last_ident;
  /* the state directory */
  char *ia_statedir;
  /* the transient data space */
  struct iacadataspace_st *ia_transientdataspace;
  /* the toplevel dictionnary */
  IacaItem *ia_topdictitm;
  /* the dataspace hook closure item */
  IacaItem *ia_dataspacehookitm;
  /* the GTK initializing closure item */
  IacaItem *ia_gtkinititm;
  /* the module dumping closure item */
  IacaItem *ia_moduledumpitm;
  /* the GTK application, if any */
  GtkApplication *ia_gtkapplication;
  /* flag for debug messages */
  gboolean ia_debug;
} iaca;

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

#define iacav_string_make(L) ((IacaValue*)iaca_string_make((L)))

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
  unsigned v_kind;		/* always IACAV_NODE */
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
#define iacav_node_makevar(Conn,...) \
  ((IacaValue*)(iaca_node_makevarf(Conn,##__VA_ARGS__,(IacaValue*)0)))

// allocator of a transient node with its array of sons; return null if no
// connective
extern IacaNode *iaca_transnode_make (IacaValue *conn, IacaValue *sontab[],
				      unsigned arity);
#define iacav_transnode_make(Con,Sons,Ari) \
  ((IacaValue*)iaca_transnode_make((Con),(Sons),(Ari)))
// variadic allocator of a node with its sons, null terminated
extern IacaNode *iaca_transnode_makevarf (IacaValue *conn, ...)
  __attribute__ ((sentinel));
#define iacav_transnode_makevar(Conn,...) \
  ((IacaValue*)(iaca_transnode_makevarf(Conn,##__VA_ARGS__,(IacaValue*)0)))

// safe accessors
// get the connective or a default
static inline IacaValue *iaca_node_conn_def (IacaValue *v, IacaValue *def);
#define iaca_node_conn(V) iaca_node_conn_def((V),(IacaValue*)0)

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
     { "kd" : "setv" ,  "elemids" : [ <element-ids...> ] } 
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
extern IacaSet *iaca_set_make (IacaValue *parentset, IacaValue *elemtab[],
			       unsigned card);
#define iacav_set_make(Par,Elem,Card) \
  ((IacaValue*)iaca_set_make((Par),(Elem),(Card)))

// variadic allocator of a set with its parent and elements, null terminated
extern IacaSet *iaca_set_makevarf (IacaValue *parentset, ...)
  __attribute__ ((sentinel));
#define iacav_set_makevar(Par,...) \
  ((IacaValue*)(iaca_set_makevarf(Par,##__VA_ARGS__,(IacaValue*)0)))
#define iacav_set_makenewvar(...) \
  ((IacaValue*)(iaca_set_makevarf((IacaValue*)0,##__VA_ARGS__,(IacaValue*)0)))

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

#define IACA_FOREACH_SET_ELEMENT_LOCAL(Set,Elem)	\
  for (IacaValue* Elem=iaca_set_first_element((Set));	\
       (Elem) != NULL;					\
       Elem = iaca_set_after_element((Set),(Elem)))

/* return the canonical empty set*/
extern IacaSet *iaca_the_empty_set (void);

/* return a singleton, or the empty set is v1 not an item */
extern IacaSet *iaca_set_singleton (IacaValue *v1);
/* return a pair, or a singleton, or an empty set ... */
extern IacaSet *iaca_set_pair (IacaValue *v1, IacaValue *v2);

/* in set operations, non-set or null values are "idempotent" */

/* union of V1 & V2 contains all elements of v1 and all of v2 */
extern IacaSet *iaca_set_union (IacaValue *v1, IacaValue *v2);
#define iacav_set_union(V1,V2) \
  ((IacaValue*)iaca_set_union((V1),(V2)))

/* intersection of V1 & V2 contains all elements both in v1 and v2 */
extern IacaSet *iaca_set_intersection (IacaValue *v1, IacaValue *v2);
#define iacav_set_intersection(V1,V2) \
  ((IacaValue*)iaca_set_intersection((V1),(V2)))

/* difference of V1 & V2 contains all elements in v1 which are not in v2 */
extern IacaSet *iaca_set_difference (IacaValue *v1, IacaValue *v2);
#define iacav_set_difference(V1,V2) \
  ((IacaValue*)iaca_set_difference((V1),(V2)))

/* symetric difference of V1 & V2 contains all elements which are
   either in v1 or in v2 but not in both */
extern IacaSet *iaca_set_symetric_difference (IacaValue *v1, IacaValue *v2);
#define iacav_set_symetric_difference(V1,V2) \
  ((IacaValue*)iaca_set_symetric_difference((V1),(V2)))


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

// safe casting to IacaSet
static inline IacaSet *iacac_set (IacaValue *v);



/***************** TRANSIENT BOXED GOBJECT VALUES ************/

struct iacagobject_st
{
  unsigned v_kind;		/* always IACAV_GOBJECT */
  GObject *v_gobject;		/* any Glib gobject - immutable */
  IacaValue *v_gdata;		/* associated data */
};

// safe casting to IacaGobject
static inline IacaGobject *iacac_gobject (IacaValue *);

// get the associated gobject or NULL
static inline GObject *iaca_gobject (IacaValue *);
#define iaca_gtkwidget(V) GTK_WIDGET(iaca_gobject(V))

// get the associated wdata or null
static inline IacaValue *iaca_gobject_data (IacaValue *);
// set the associated gdata
static inline void iaca_gobject_put_data (IacaValue *val, IacaValue *gdata);

// retrieve or make the boxed gobject of a gtk gobject
extern IacaGobject *iaca_gobject_box (GObject *gob);
#define iacav_gobject_box(Wid) ((IacaValue*)iaca_gobject_box(Wid))


/***************** SHARED ITEM VALUES ****************/
/****
  JSON representation. We serialize separately item shared references
  (= pointers to IacaItem) and item contents (=the data inside a
  IacaItem). 
  Item references are serialized as
  { "kd" : "itrv" ,  "id" : <id-number> } 

  Item contents are serialized like
  { "item" : <id-number> ,
    "itemattrs" : [ <item-attribute...> ] ,
    "itempayload" : <item-payload> 
  }

  Each <item-attribute> is a JSON object like  
  { "atid" : <attribute-id-number> ,
    "val" : <value> }
  
  

***/

/* item payload */
enum iacapayloadkind_en
{
  IACAPAYLOAD__NONE,
  IACAPAYLOAD_BUFFER,
  IACAPAYLOAD_VECTOR,
  IACAPAYLOAD_DICTIONNARY,
  IACAPAYLOAD_QUEUE,
  IACAPAYLOAD_CLOSURE,
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
  struct iacadataspace_st *v_dataspace;	/* the item data space defines where it is
					   persisted */
  /* anonymous union of pointers for pay load. Change simultanously
     the v_payloadkind. */
  union
  {
    void *v_payloadptr;		/* null when IACAPAYLOAD__NONE */
    struct iacapayloadbuffer_st *v_payloadbuf;	/* when IACAPAYLOAD_BUFFER */
    struct iacapayloadvector_st *v_payloadvect;	/* when IACAPAYLOAD_VECTOR */
    struct iacapayloaddictionnary_st *v_payloaddict;	/* when IACAPAYLOAD_DICTIONNARY */
    struct iacapayloadqueue_st *v_payloadqueue;	/* when IACAPAYLOAD_QUEUE */
    struct iacapayloadclosure_st *v_payloadclosure;	/* when IACAPAYLOAD_CLOSURE */
  };
};
// safe casting to IacaItem
static inline IacaItem *iacac_item (IacaValue *v);

static inline int64_t iaca_item_ident (IacaItem *it);
#define iaca_item_identll(Itm) ((long long) iaca_item_ident((Itm)))
struct iacapayloadvector_st
{
  unsigned vec_siz;		/* allocated size */
  unsigned vec_len;		/* used length */
  IacaValue *vec_tab[];		/* vec_siz pointers, only the first
				   vec_len are used */
};

struct iacapayloadbuffer_st
{
  unsigned buf_siz;		/* allocated size */
  unsigned buf_len;		/* used length */
  char buf_tab[];		/* buf_siz bytes */
};

struct iacadictentry_st
{
  IacaString *de_str;
  IacaValue *de_val;
};

struct iacapayloaddictionnary_st
{
  unsigned dic_siz;
  unsigned dic_len;
  struct iacadictentry_st dic_tab[];	/* size is dic_siz */
};

struct iacaqueuelink_st
{
  struct iacaqueuelink_st *ql_prev;
  struct iacaqueuelink_st *ql_next;
  IacaValue *ql_val;
};

struct iacapayloadqueue_st
{
  unsigned que_len;
  struct iacaqueuelink_st *que_first;
  struct iacaqueuelink_st *que_last;
};


#define IACA_CLOFUN_MAGIC 0x5285cd1f	/*1384500511 */

/* closure function signatures defines the argument and result types
   of a function, hence the code context in which it is called. The
   name after the IACACFSIG_ prefix is in lower case to match the
   union member inside iacaclofun_st */
enum iacaclofunsig_en
{
  IACACFSIG__NONE,		/* never used */
  /* activation or other action signal of a GObject, or even of some
     GtkWidget like GtkButton... */
  IACACFSIG_gobject_do,
  /* application to one value */
  IACACFSIG_one_value,
  /* application to two values */
  IACACFSIG_two_values,
  /* application to three values */
  IACACFSIG_three_values,
  /* computed attribute */
  IACACFSIG_computed_attribute,
};

/* closure function structures are always const and statically
   allocated, that is in text segment. A clofun whose cfun_name is FOO
   is named iacacfun_FOO */
struct iacaclofun_st
{
  const unsigned cfun_magic;	/* always IACA_CLOFUN_MAGIC */
  /* The number of closed values in the containing closure: */
  const unsigned cfun_nbval;
  /* the signature of the function */
  enum iacaclofunsig_en cfun_sig;
  /* The module name is the basename of the C file containing the code:  */
  const char *cfun_module;
  /* The pointer is a function pointer: */
  union
  {
    /* when IACACFSIG__NONE, not really used: */
    void *cfun_ptr;

    struct
    {
      void *cfun_ptr1;
      void *cfun_ptr2;
    };

    /* when IACACFSIG_gobject_do (gobj,cloitm); activation or other
       action signal of a GObject; the widget is a button, a menu
       item, ... and the item contains the closure; the resulting
       function is valid as a gtk signal for the "activate" signal of
       the button, menu item, ... */
    void (*cfunu_gobject_do) (GObject *, IacaItem *);
#define cfun_gobject_do cfun_un.cfunu_gobject_do
    /* when IACACFSIG_one_value (v1, cloitm) */
    IacaValue *(*cfunu_one_value) (IacaValue *, IacaItem *);
#define cfun_one_value cfun_un.cfunu_one_value
    /* when IACACFSIG_two_values (v1, v2, cloitm) */
    IacaValue *(*cfunu_two_values) (IacaValue *, IacaValue *, IacaItem *);
#define cfun_two_values cfun_un.cfunu_two_values
    /* when IACACFSIG_three_values (v1, v2, v3, cloitm) */
    IacaValue *(*cfunu_three_values) (IacaValue *, IacaValue *, IacaValue *,
				      IacaItem *);
#define cfun_three_values cfun_un.cfunu_three_values
    struct
    {
      IacaValue *(*cfunu_get_attribute) (IacaItem *, IacaItem *, IacaValue *);
      void (*cfunu_put_attribute) (IacaItem *, IacaItem *, IacaValue *);
    } cfunu_attrfun;
#define cfun_get_attribute cfun_un.cfunu_attrfun.cfunu_get_attribute
#define cfun_put_attribute cfun_un.cfunu_attrfun.cfunu_put_attribute
  } cfun_un;
  /* The name is FOO when the entire iacaclofun_st structure is
     iacafun_FOO: */
  const char cfun_name[];
};

#define IACA_DEFINE_CLOFUN(Name,Nbval,Sig,Fun)		\
  const struct iacaclofun_st iacacfun_##Name = {	\
    .cfun_magic = IACA_CLOFUN_MAGIC,			\
    .cfun_nbval = Nbval,				\
    .cfun_sig = IACACFSIG_##Sig,			\
    .cfun_module = IACA_MODULE,				\
    .cfun_##Sig = Fun,					\
    .cfun_name = #Name					\
}

extern const struct iacaclofun_st *iaca_find_clofun (const char *name);

struct iacapayloadclosure_st
{
  const struct iacaclofun_st *clo_fun;
  IacaValue *clo_valtab[];	/* size is given by cfun_nbval of clo_fun */
};

////////////////



/* make an item */
extern IacaItem *iaca_item_make (struct iacadataspace_st *sp);
#define iacav_item_make(Sp) ((IacaValue*) iaca_item_make((Sp)))

static inline int
iaca_item_compare (IacaItem *it1, IacaItem *it2)
{
  if (it1 == it2)
    return 0;
  if (!it1)
    return -1;
  if (!it2)
    return 1;
  g_assert (it1->v_ident != it2->v_ident);
  if (it1->v_ident < it2->v_ident)
    return -1;
  else				/*if (it1->v_ident > it2->v_ident) */
    return -1;
}

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
static inline IacaValue *iaca_item_physical_get_def (IacaValue *vitem,
						     IacaValue *vattr,
						     IacaValue *vdef);

#define iaca_item_physical_get(Vitem,Vattr) \
  iaca_item_physical_get_def((Vitem),(Vattr),(IacaValue*)0)

/* in item vitem, logically get the value associate to attribute vattr
   or else default vdef; if the attribute is absent and has a getting
   routine, use it */
static inline IacaValue *iaca_item_get_def (IacaValue *vitem,
					    IacaValue *vattr,
					    IacaValue *vdef);

#define iaca_item_get(Vitem,Vattr) \
  iaca_item_get_def((Vitem),(Vattr),(IacaValue*)0)

static inline IacaValue *iaca_item_content_def (IacaValue *vitem,
						IacaValue *vdef);

#define iaca_item_content(Vitem) iaca_item_content_def ((Vitem),(IacaValue*)0)

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

static inline int iaca_item_number_attributes (IacaValue *vitem);

/* iterate inside an item attribute; Item and Attr should be local
   variables; Item should not be modified inside the for body. */
#define IACA_FOREACH_ITEM_ATTRIBUTE(Vitem,Vattr)           \
  for(Vattr=iaca_item_first_attribute ((Vitem));           \
      (Vattr) != NULL;                                     \
      Vattr = iaca_item_next_attribute((Vitem),(Vattr)))

#define IACA_FOREACH_ITEM_ATTRIBUTE_LOCAL(Vitem,Vattr)          \
  for (IacaValue* Vattr=iaca_item_first_attribute ((Vitem));    \
       (Vattr) != NULL;                                         \
       Vattr = iaca_item_next_attribute((Vitem),                \
                                       (Vattr)))

static inline enum iacapayloadkind_en iaca_item_pay_load_kind (IacaItem *itm);

static inline void iaca_item_pay_load_clear (IacaItem *itm);

extern void iaca_item_pay_load_resize_vector (IacaItem *itm, unsigned sz);

static inline unsigned iaca_item_pay_load_vector_length (IacaItem *itm);

static inline IacaValue *iaca_item_pay_load_nth_vector (IacaItem *itm,
							int rk);

static inline void iaca_item_pay_load_put_nth_vector (IacaItem *itm, int rk,
						      IacaValue *val);

extern void iaca_item_pay_load_append_vector (IacaItem *itm, IacaValue *val);

extern void iaca_item_pay_load_reserve_buffer (IacaItem *itm, unsigned sz);

extern void iaca_item_pay_load_append_buffer (IacaItem *itm, const char *str);

extern void
iaca_item_pay_load_append_cencoded_buffer (IacaItem *itm, const char *str);

extern void iaca_item_pay_load_bufprintf (IacaItem *itm, const char *fmt,
					  ...)
  __attribute__ ((format (printf, 2, 3)));

static inline unsigned iaca_item_pay_load_buffer_length (IacaItem *itm);

extern void iaca_item_pay_load_reserve_dictionnary (IacaItem *itm,
						    unsigned sz);

static inline IacaValue *iaca_item_pay_load_dictionnary_get
  (IacaItem *itm, const char *name);

static inline IacaString *iaca_item_pay_load_dictionnary_next_string
  (IacaItem *itm, const char *name);

#define IACA_FOREACH_DICTIONNARY_STRING_LOCAL(Itdic,Strvar)	\
  for (IacaString* Strvar =					\
       iaca_item_pay_load_dictionnary_next_string		\
       (Itdic, NULL);						\
     Strvar != NULL;						\
     Strvar =							\
       iaca_item_pay_load_dictionnary_next_string		\
       (Itdic,							\
	iaca_string_val((IacaValue*)(Strvar))))

extern void
iaca_item_pay_load_put_dictionnary (IacaItem *itm, IacaString *strv,
				    IacaValue *val);

static inline void
iaca_item_pay_load_put_dictionnary_str (IacaItem *itm, const char *str,
					IacaValue *val)
{
  IacaString *strv = 0;
  if (!itm || itm->v_kind != IACAV_ITEM
      || !str || !str[0]
      || itm->v_payloadkind != IACAPAYLOAD_DICTIONNARY || !val)
    {
      iaca_debug
	("fail iaca_item_pay_load_put_dict_str itm %p #%lld str '%s' val %p",
	 itm, iaca_item_identll (itm), str, val);
      return;
    }
  strv = iaca_string_make (str);
  iaca_item_pay_load_put_dictionnary (itm, strv, val);
}

extern void
iaca_item_pay_load_remove_dictionnary_str (IacaItem *itm, const char *str);

extern void iaca_item_pay_load_make_queue (IacaItem *itm);

extern void iaca_item_pay_load_queue_prepend (IacaItem *itm, IacaValue *val);

extern void iaca_item_pay_load_queue_append (IacaItem *itm, IacaValue *val);


static inline IacaValue *
iaca_item_pay_load_queue_first (IacaItem *itm)
{
  struct iacapayloadqueue_st *que = 0;
  struct iacaqueuelink_st *lnkfirst = 0;
  if (!itm || itm->v_kind != IACAV_ITEM)
    return NULL;
  if (itm->v_payloadkind != IACAPAYLOAD_QUEUE
      || (que = itm->v_payloadqueue) == NULL)
    return NULL;
  if (que->que_len == 0)
    return NULL;
  lnkfirst = que->que_first;
  g_assert (lnkfirst != NULL);
  g_assert (lnkfirst->ql_prev == NULL);
  return lnkfirst->ql_val;
}


static inline IacaValue *
iaca_item_pay_load_queue_last (IacaItem *itm)
{
  struct iacapayloadqueue_st *que = 0;
  struct iacaqueuelink_st *lnklast = 0;
  if (!itm || itm->v_kind != IACAV_ITEM)
    return NULL;
  if (itm->v_payloadkind != IACAPAYLOAD_QUEUE
      || (que = itm->v_payloadqueue) == NULL)
    return NULL;
  if (que->que_len == 0)
    return NULL;
  lnklast = que->que_last;
  g_assert (lnklast != NULL);
  g_assert (lnklast->ql_next == NULL);
  return lnklast->ql_val;
}

extern void
iaca_item_pay_load_make_closure (IacaItem *itm,
				 const struct iacaclofun_st *cfun,
				 IacaValue **valtab);

/* can be passed to g_signal_connect */
extern void
iaca_item_pay_load_closure_gobject_do (GObject *gob, IacaItem *cloitm);



static inline IacaValue *
iaca_item_pay_load_closure_one_value (IacaValue *v1, IacaItem *cloitm)
{
  const struct iacaclofun_st *cfun = 0;
  struct iacapayloadclosure_st *clo = 0;
  if (!cloitm || cloitm->v_kind != IACAV_ITEM)
    return NULL;
  if (cloitm->v_payloadkind != IACAPAYLOAD_CLOSURE
      || !(clo = cloitm->v_payloadclosure))
    return NULL;
  if (!(cfun = clo->clo_fun))
    return NULL;
  g_assert (cfun->cfun_magic == IACA_CLOFUN_MAGIC);
  if (cfun->cfun_sig != IACACFSIG_one_value || !cfun->cfun_one_value)
    return NULL;
  return cfun->cfun_one_value (v1, cloitm);
}


static inline IacaValue *
iaca_item_pay_load_closure_two_values (IacaValue *v1, IacaValue *v2,
				       IacaItem *cloitm)
{
  const struct iacaclofun_st *cfun = 0;
  struct iacapayloadclosure_st *clo = 0;
  if (!cloitm || cloitm->v_kind != IACAV_ITEM)
    return NULL;
  if (cloitm->v_payloadkind != IACAPAYLOAD_CLOSURE
      || !(clo = cloitm->v_payloadclosure))
    return NULL;
  if (!(cfun = clo->clo_fun))
    return NULL;
  g_assert (cfun->cfun_magic == IACA_CLOFUN_MAGIC);
  if (cfun->cfun_sig != IACACFSIG_two_values || !cfun->cfun_two_values)
    return NULL;
  return cfun->cfun_two_values (v1, v2, cloitm);
}


static inline IacaValue *
iaca_item_pay_load_closure_three_values (IacaValue *v1, IacaValue *v2,
					 IacaValue *v3, IacaItem *cloitm)
{
  const struct iacaclofun_st *cfun = 0;
  struct iacapayloadclosure_st *clo = 0;
  if (!cloitm || cloitm->v_kind != IACAV_ITEM)
    return NULL;
  if (cloitm->v_payloadkind != IACAPAYLOAD_CLOSURE
      || !(clo = cloitm->v_payloadclosure))
    return NULL;
  if (!(cfun = clo->clo_fun))
    return NULL;
  g_assert (cfun->cfun_magic == IACA_CLOFUN_MAGIC);
  if (cfun->cfun_sig != IACACFSIG_three_values || !cfun->cfun_three_values)
    return NULL;
  return cfun->cfun_three_values (v1, v2, v3, cloitm);
}

/* the number of arguments should correspond to cfun */
extern void
iaca_item_pay_load_make_closure_varf (IacaItem *itm,
				      const struct iacaclofun_st *cfun, ...);

static inline IacaValue *
iaca_item_pay_load_closure_nth (IacaItem *itm, int rk)
{
  struct iacapayloadclosure_st *clo = 0;
  const struct iacaclofun_st *cfun = 0;
  unsigned nbv = 0;
  if (!itm || itm->v_kind != IACAV_ITEM)
    return NULL;
  if (itm->v_payloadkind != IACAPAYLOAD_CLOSURE
      || (clo = itm->v_payloadclosure) == NULL
      || (cfun = clo->clo_fun) == NULL || (nbv = cfun->cfun_nbval) == 0)
    return NULL;
  if (rk < 0)
    rk += (int) nbv;
  if (rk >= 0 && rk < (int) nbv)
    return clo->clo_valtab[rk];
  return NULL;
}

static inline void
iaca_item_pay_load_closure_set_nth (IacaItem *itm, int rk, IacaValue *val)
{
  struct iacapayloadclosure_st *clo = 0;
  const struct iacaclofun_st *cfun = 0;
  unsigned nbv = 0;
  if (!itm || itm->v_kind != IACAV_ITEM)
    return;
  if (itm->v_payloadkind != IACAPAYLOAD_CLOSURE
      || (clo = itm->v_payloadclosure) == NULL
      || (cfun = clo->clo_fun) == NULL || (nbv = cfun->cfun_nbval) == 0)
    return;
  if (rk < 0)
    rk += (int) nbv;
  if (rk >= 0 && rk < (int) nbv)
    clo->clo_valtab[rk] = val;
}


////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/* recursively copy a value (and the sons of nodes) */
extern IacaValue *iaca_copy (IacaValue *);

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

// safe casting to IacaSet
static inline IacaSet *
iacac_set (IacaValue *v)
{
  if (!v || v->v_kind != IACAV_SET)
    return NULL;
  return ((IacaSet *) v);
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
iaca_item_physical_get_def (IacaValue *vitem, IacaValue *vattr,
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


static inline IacaValue *
iaca_item_content_def (IacaValue *vitem, IacaValue *vdef)
{
  IacaItem *item = 0;
  if (!vitem || vitem->v_kind != IACAV_ITEM)
    return vdef;
  item = (IacaItem *) vitem;
  return item->v_itemcontent;
}

static inline IacaValue *
iaca_item_get_def (IacaValue *vitem, IacaValue *vattr, IacaValue *vdef)
{
  IacaItem *item = 0;
  IacaItem *attr = 0;
  struct iacatabattr_st *tbl = 0;
  const struct iacaclofun_st *cfun = 0;
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
    {
      if (attr->v_payloadkind == IACAPAYLOAD_CLOSURE
	  && attr->v_payloadclosure
	  && (cfun = attr->v_payloadclosure->clo_fun)
	  && cfun->cfun_magic == IACA_CLOFUN_MAGIC
	  && cfun->cfun_sig == IACACFSIG_computed_attribute
	  && cfun->cfun_get_attribute)
	return cfun->cfun_get_attribute (item, attr, vdef);
      else
	return vdef;
    }
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

// safe casting to IacaItem
static inline IacaItem *
iacac_item (IacaValue *v)
{
  if (!v || v->v_kind != IACAV_ITEM)
    return NULL;
  return ((IacaItem *) v);
}

static inline int64_t
iaca_item_ident (IacaItem *it)
{
  return (it && it->v_kind == IACAV_ITEM) ? it->v_ident : 0;
}

static inline enum iacapayloadkind_en
iaca_item_pay_load_kind (IacaItem *itm)
{
  if (!itm || itm->v_kind != IACAV_ITEM)
    return IACAPAYLOAD__NONE;
  return itm->v_payloadkind;
}

static inline void
iaca_item_pay_load_clear (IacaItem *itm)
{
  if (!itm || itm->v_kind != IACAV_ITEM)
    return;
  itm->v_payloadkind = IACAPAYLOAD__NONE;
  itm->v_payloadptr = NULL;
}


static inline unsigned
iaca_item_pay_load_vector_length (IacaItem *itm)
{
  struct iacapayloadvector_st *pv = 0;
  if (!itm || itm->v_kind != IACAV_ITEM
      || itm->v_payloadkind != IACAPAYLOAD_VECTOR
      || (pv = itm->v_payloadvect) == NULL)
    return 0;
  return pv->vec_len;
}

static inline unsigned
iaca_item_pay_load_buffer_length (IacaItem *itm)
{
  struct iacapayloadbuffer_st *buf = 0;
  if (!itm || itm->v_kind != IACAV_ITEM
      || itm->v_payloadkind != IACAPAYLOAD_VECTOR
      || (buf = itm->v_payloadbuf) == NULL)
    return 0;
  return buf->buf_len;
}

static inline const char *
iaca_item_pay_load_buffer_str (IacaItem *itm)
{
  struct iacapayloadbuffer_st *buf = 0;
  if (!itm || itm->v_kind != IACAV_ITEM
      || itm->v_payloadkind != IACAPAYLOAD_VECTOR
      || (buf = itm->v_payloadbuf) == NULL)
    return NULL;
  return buf->buf_tab;
}

static inline IacaValue *
iaca_item_pay_load_nth_vector (IacaItem *itm, int rk)
{
  struct iacapayloadvector_st *pv = 0;
  if (!itm || itm->v_kind != IACAV_ITEM
      || itm->v_payloadkind != IACAPAYLOAD_VECTOR
      || (pv = itm->v_payloadvect) == NULL)
    return NULL;
  if (rk < 0)
    rk += pv->vec_len;
  if (rk >= 0 && rk < pv->vec_len)
    return pv->vec_tab[rk];
  return NULL;
}

static inline void
iaca_item_pay_load_put_nth_vector (IacaItem *itm, int rk, IacaValue *val)
{
  struct iacapayloadvector_st *pv = 0;
  if (!itm || itm->v_kind != IACAV_ITEM
      || itm->v_payloadkind != IACAPAYLOAD_VECTOR
      || (pv = itm->v_payloadvect) == NULL)
    return;
  if (rk < 0)
    rk += pv->vec_len;
  if (rk >= 0 && rk < pv->vec_len)
    pv->vec_tab[rk] = val;
}

static inline IacaValue *
iaca_item_pay_load_dictionnary_get (IacaItem *itm, const char *name)
{
  int lo = 0, hi = 0, md = 0;
  struct iacapayloaddictionnary_st *dic = 0;
  if (!itm || itm->v_kind != IACAV_ITEM
      || !name || !name[0]
      || itm->v_payloadkind != IACAPAYLOAD_DICTIONNARY
      || (dic = itm->v_payloaddict) == NULL)
    return NULL;
  lo = 0;
  hi = (int) dic->dic_len - 1;
  while (lo + 1 < hi)
    {
      IacaString *str = 0;
      int cmp = 0;
      md = (lo + hi) / 2;
      str = dic->dic_tab[md].de_str;
      g_assert (str != 0 && str->v_kind == IACAV_STRING);
      cmp = strcmp (str->v_str, name);
      if (!cmp)
	return dic->dic_tab[md].de_val;
      if (cmp < 0)
	hi = md;
      else
	lo = md;
    };
  for (md = lo; md <= hi; md++)
    {
      IacaString *str = 0;
      int cmp = 0;
      str = dic->dic_tab[md].de_str;
      g_assert (str != 0);
      cmp = strcmp (str->v_str, name);
      if (!cmp)
	return dic->dic_tab[md].de_val;
    }
  return NULL;
}

static inline IacaString *
iaca_item_pay_load_dictionnary_next_string (IacaItem *itm, const char *name)
{
  int lo = 0, hi = 0, md = 0, ln = 0;
  struct iacapayloaddictionnary_st *dic = 0;
  if (!itm || itm->v_kind != IACAV_ITEM
      || itm->v_payloadkind != IACAPAYLOAD_DICTIONNARY
      || (dic = itm->v_payloaddict) == NULL)
    return NULL;
  lo = 0;
  ln = (int) dic->dic_len;
  hi = ln - 1;
  if (!name || !name[0])
    {
      if (hi >= 0)
	return dic->dic_tab[0].de_str;
      return NULL;
    }
  while (lo + 1 < hi)
    {
      IacaString *str = 0;
      int cmp = 0;
      md = (lo + hi) / 2;
      str = dic->dic_tab[md].de_str;
      g_assert (str != 0 && str->v_kind == IACAV_STRING);
      cmp = strcmp (str->v_str, name);
      if (!cmp)
	{
	  if (md + 1 < ln)
	    return dic->dic_tab[md + 1].de_str;
	  else
	    return NULL;
	}
      if (cmp < 0)
	hi = md;
      else
	lo = md;
    };
  for (md = lo; md <= hi; md++)
    {
      IacaString *str = 0;
      int cmp = 0;
      str = dic->dic_tab[md].de_str;
      g_assert (str != 0);
      cmp = strcmp (str->v_str, name);
      if (!cmp)
	{
	  if (md + 1 < ln)
	    return dic->dic_tab[md + 1].de_str;
	  else
	    return NULL;
	}
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
  for (ix = ix + 1; ix < (int) sz; ix++)
    {
      IacaItem *curit = tbl->at_entab[ix].en_item;
      if (!curit || curit == IACA_EMPTY_SLOT)	/* emptied slot */
	continue;
      return (IacaValue *) curit;
    }
  return NULL;
}


static inline int
iaca_item_number_attributes (IacaValue *vitem)
{
  IacaItem *item = 0;
  struct iacatabattr_st *tbl = 0;
  if (!vitem || vitem->v_kind != IACAV_ITEM)
    return 0;
  item = (IacaItem *) vitem;
  tbl = item->v_attrtab;
  if (!tbl)
    return 0;
  return tbl->at_count;
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
  int lo = 0, hi = 0, md = 0;
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
  hi = (int) (set->v_cardinal) - 1;
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


/************* BOXED GOBJECT VALUES ***************/
static inline IacaGobject *
iacac_gobject (IacaValue *val)
{
  if (!val || val->v_kind != IACAV_GOBJECT)
    return NULL;
  return (IacaGobject *) val;
}

static inline GObject *
iaca_gobject (IacaValue *val)
{
  if (!val || val->v_kind != IACAV_GOBJECT)
    return NULL;
  return ((IacaGobject *) val)->v_gobject;
}

static inline IacaValue *
iaca_gobject_data (IacaValue *val)
{
  if (!val || val->v_kind != IACAV_GOBJECT)
    return NULL;
  return ((IacaGobject *) val)->v_gdata;
}

static inline void
iaca_gobject_put_data (IacaValue *val, IacaValue *wdata)
{
  if (!val || val->v_kind != IACAV_GOBJECT)
    return;
  ((IacaGobject *) val)->v_gdata = wdata;
}


/* the name of the manifest file, referencing other files in the state
   directory */
#define IACA_MANIFEST_FILE "IaCa_Manifest"

#define IACA_SPACE_MAGIC 0x67467f0b	/*1732673291 */
/* dataspaces are not garbage collected! */
struct iacadataspace_st
{
  unsigned dsp_magic;		/* always IACA_SPACE_MAGIC */
  /* GC-ed space name */
  IacaString *dsp_name;
};


struct iacadataspace_st *iaca_dataspace (const char *name);

/* load the entire state from a directory */
void iaca_load (const char *dirpath);

/* dump the entire state into a directory */
void iaca_dump (const char *dirpath);

/* load a [binary] module of a given name which should contain only
   letters from A-Z a-z, digits 0-9 or underscore _. Returns NULL on
   success, or a GC_strdup-ed error string on failure. */
const char *iaca_load_module (const char *dirpath, const char *modnam);
#endif /*IACA_INCLUDED */
