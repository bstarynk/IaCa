
/****
  © 2011 Basile Starynkevitch 
    this file main.c is part of IaCa

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


struct iaca_st iaca;

IacaNode *
iaca_node_make (IacaValue *conn, IacaValue *sontab[], unsigned arity)
{
  IacaNode *n = 0;
  size_t sz = 0;
  if (!conn || conn->v_kind != IACAV_ITEM)
    return NULL;
  g_assert (arity < IACA_ARITY_MAX);
  sz = sizeof (IacaNode) + (arity + 1) * sizeof (IacaValue *);
  n = iaca_alloc_data (sz);
  n->v_kind = IACAV_NODE;
  n->v_arity = arity;
  n->v_conn = (IacaItem *) conn;
  if (sontab)
    memcpy (n->v_sons, sontab, arity * sizeof (IacaValue *));
  return n;
}

IacaNode *
iaca_node_makevarf (IacaValue *conn, ...)
{
  IacaNode *n = 0;
  size_t sz = 0;
  unsigned ar = 0;
  va_list arglist;
  if (!conn || conn->v_kind != IACAV_ITEM)
    return NULL;
  va_start (arglist, conn);
  while (va_arg (arglist, IacaValue *) != NULL)
      ar++;
  va_end (arglist);
  g_assert (ar < IACA_ARITY_MAX);
  sz = sizeof (IacaNode) + (ar + 1) * sizeof (IacaValue *);
  n = iaca_alloc_data (sz);
  n->v_kind = IACAV_NODE;
  n->v_arity = ar;
  n->v_conn = (IacaItem *) conn;
  va_start (arglist, conn);
  for (unsigned ix = 0; ix < ar; ix++)
    n->v_sons[ix] = va_arg (arglist, IacaValue *);
  va_end (arglist);
  return n;
}


static const struct iacaset_st iaca_empty_setdata = {
  .v_kind = IACAV_SET,
  .v_cardinal = 0,
  .v_elements = {}
};

IacaSet *
iaca_the_empty_set (void)
{
  return (IacaSet *) &iaca_empty_setdata;
}

static int
iaca_itemptr_compare (const void *p1, const void *p2)
{
  IacaItem *i1 = *(IacaItem **) p1;
  IacaItem *i2 = *(IacaItem **) p2;
  if (i1 == i2)
    return 0;
  if (i1->v_ident < i2->v_ident)
    return -1;
  else
    return 1;
}

/* we allocate on stack intermediate space for small sets, otherwise in heap */
#define IACA_QUICK_MAX_CARDINAL 8

/* internal function to make a set by merging two ordered array of items */
static IacaSet *
iaca_unsafe_set_make_merge (IacaItem *tab1[], unsigned cnt1,
			    IacaItem *tab2[], unsigned cnt2)
{
  unsigned ix1 = 0, ix2 = 0, cnt = 0;
  unsigned newsiz = cnt1 + cnt2;
  IacaSet *newset =
    iaca_alloc_data (sizeof (IacaSet) + newsiz * sizeof (IacaItem *));
  for (;;)
    {
      IacaItem *it1 = (ix1 < cnt1) ? tab1[ix1] : 0;
      IacaItem *it2 = (ix1 < cnt2) ? tab2[ix2] : 0;
      IacaItem *prevelem = (cnt > 0) ? newset->v_elements[cnt - 1] : 0;
      if (!it1 && !it2)
	break;
      if (!it1)
	{
	  if (prevelem != it2)
	    newset->v_elements[cnt++] = it2;
	  ix2++;
	  continue;
	}
      if (!it2)
	{
	  if (prevelem != it1)
	    newset->v_elements[cnt++] = it1;
	  ix1++;
	  continue;
	}
      if (it1 == it2)
	{
	  if (prevelem != it1)
	    newset->v_elements[cnt++] = it1;
	  ix1++;
	  ix2++;
	  continue;
	}
      if (it1->v_ident < it2->v_ident)
	{
	  if (prevelem != it1)
	    newset->v_elements[cnt++] = it1;
	  ix1++;
	  continue;
	}
      else
	{
	  if (prevelem != it2)
	    newset->v_elements[cnt++] = it2;
	  ix2++;
	  continue;
	}
    }
  if (cnt + 1 < 7 * newsiz / 8)
    {				/* newset allocated too big! */
      IacaSet *bigset = newset;
      newset = iaca_alloc_data (sizeof (IacaSet) + cnt * sizeof (IacaItem *));
      for (unsigned ix = 0; ix < cnt; ix++)
	newset->v_elements[ix] = bigset->v_elements[ix];
      GC_FREE (bigset);
    };
  newset->v_kind = IACAV_SET;
  newset->v_cardinal = cnt;
  return newset;
}


IacaSet *
iaca_set_singleton (IacaValue *v1)
{
  IacaSet *set = 0;
  IacaItem *it1 = (v1 && v1->v_kind == IACAV_ITEM) ? ((IacaItem *) v1) : 0;
  if (it1)
    {
      set = iaca_alloc_data (sizeof (IacaSet) + sizeof (IacaItem *));
      set->v_kind = IACAV_SET;
      set->v_cardinal = 1;
      set->v_elements[0] = it1;
      return set;
    }
  else
    return (IacaSet *) &iaca_empty_setdata;
}

IacaSet *
iaca_set_pair (IacaValue *v1, IacaValue *v2)
{
  IacaSet *set = 0;
  IacaItem *it1 = (v1 && v1->v_kind == IACAV_ITEM) ? ((IacaItem *) v1) : 0;
  IacaItem *it2 = (v2 && v2->v_kind == IACAV_ITEM) ? ((IacaItem *) v2) : 0;
  if (!it1 && !it2)
    return (IacaSet *) &iaca_empty_setdata;
  if (!it2 || it1 == it2)
    {
      set = iaca_alloc_data (sizeof (IacaSet) + sizeof (IacaItem *));
      set->v_elements[0] = it1;
      set->v_kind = IACAV_SET;
      set->v_cardinal = 1;
      return set;
    }
  else if (!it1)
    {
      set = iaca_alloc_data (sizeof (IacaSet) + sizeof (IacaItem *));
      set->v_elements[0] = it2;
      set->v_kind = IACAV_SET;
      set->v_cardinal = 1;
      return set;
    }
  set = iaca_alloc_data (sizeof (IacaSet) + 2 * sizeof (IacaItem *));
  if (it1->v_ident < it2->v_ident)
    {
      set->v_elements[0] = it1;
      set->v_elements[1] = it2;
    }
  else
    {
      set->v_elements[0] = it2;
      set->v_elements[1] = it1;
    }
  set->v_kind = IACAV_SET;
  set->v_cardinal = 2;
  return set;
}


IacaSet *
iaca_set_make (IacaValue *parent, IacaValue *elemtab[], unsigned card)
{
  IacaSet *newset = 0;
  IacaSet *parentset = 0;
  IacaItem **newitems = 0;
  unsigned nbelems = 0;
  IacaItem *quitems[IACA_QUICK_MAX_CARDINAL] = { 0 };
  if (!parent || parent->v_kind != IACAV_SET)
    parentset = NULL;
  else
    parentset = (IacaSet *) parent;
  /* fill newitems with a sequence of item elements, using quitems if
     few enought to avoid memory allocation */
  if (card < IACA_QUICK_MAX_CARDINAL)
    newitems = quitems;
  else
    newitems = iaca_alloc_data (sizeof (IacaItem *) * card);
  for (unsigned i = 0; i < card; i++)
    {
      IacaValue *curelem = elemtab[i];
      if (curelem && curelem->v_kind == IACAV_ITEM)
	newitems[nbelems++] = (IacaItem *) curelem;
    };
  if (nbelems == 0)
    {
      if (parentset)
	return parentset;
      return (IacaSet *) &iaca_empty_setdata;
    }
  else if (nbelems == 1)
    {
    }
  else if (nbelems == 2)
    {
      if (newitems[0] == newitems[1])
	nbelems = 1;
      else if (newitems[0]->v_ident > newitems[1]->v_ident)
	{
	  IacaItem *it0 = newitems[0];
	  IacaItem *it1 = newitems[1];
	  newitems[0] = it1;
	  newitems[1] = it0;
	}
    }
  else
    /* sort the newitems */
    qsort (newitems, nbelems, sizeof (IacaItem *), iaca_itemptr_compare);
  newset = iaca_unsafe_set_make_merge (parentset->v_elements,
				       parentset->v_cardinal,
				       newitems, nbelems);
  return newset;
}


IacaSet *
iaca_set_makevarf (IacaValue *parent, ...)
{
  IacaSet *newset = 0;
  unsigned cnt = 0;
  IacaValue *quvals[IACA_QUICK_MAX_CARDINAL] = { 0 };
  IacaSet *parentset = 0;
  IacaValue *curval = 0;
  IacaValue **vals = 0;
  va_list args;
  if (!parent || parent->v_kind != IACAV_SET)
    parentset = NULL;
  else
    parentset = (IacaSet *) parent;
  va_start (args, parent);
  while ((curval = va_arg (args, IacaValue *)) != 0)
    {
      if (cnt < IACA_QUICK_MAX_CARDINAL)
	quvals[cnt] = curval;
      cnt++;
    }
  va_end (args);
  if (cnt < IACA_QUICK_MAX_CARDINAL)
    newset = iaca_set_make ((IacaValue *) parentset, quvals, cnt);
  else
    {
      vals = iaca_alloc_data (cnt * sizeof (IacaValue *));
      cnt = 0;
      va_start (args, parent);
      while ((curval = va_arg (args, IacaValue *)) != 0)
	  vals[cnt++] = curval;
      va_end (args);
      newset = iaca_set_make ((IacaValue *) parentset, vals, cnt);
      GC_FREE (vals);
    }
  return newset;
}


IacaSet *
iaca_set_union (IacaValue *v1, IacaValue *v2)
{
  IacaSet *set1 = (v1 && v1->v_kind == IACAV_SET) ? ((IacaSet *) v1) : 0;
  IacaSet *set2 = (v2 && v2->v_kind == IACAV_SET) ? ((IacaSet *) v2) : 0;
  if (!set1 && !set2)
    return (IacaSet *) &iaca_empty_setdata;
  if (!set1 || set1->v_cardinal == 0)
    return set2;
  if (!set2 || set2->v_cardinal == 0)
    return set1;
  if (set1->v_cardinal == 1
      && iaca_set_contains ((IacaValue *) set2,
			    (IacaValue *) (set1->v_elements[0])))
    return set2;
  if (set2->v_cardinal == 1
      && iaca_set_contains ((IacaValue *) set1,
			    (IacaValue *) (set2->v_elements[0])))
    return set1;
  return
    iaca_unsafe_set_make_merge (set1->v_elements, set1->v_cardinal,
				set2->v_elements, set2->v_cardinal);
}


IacaSet *
iaca_set_intersection (IacaValue *v1, IacaValue *v2)
{
  IacaSet *set1 = (v1 && v1->v_kind == IACAV_SET) ? ((IacaSet *) v1) : 0;
  IacaSet *set2 = (v2 && v2->v_kind == IACAV_SET) ? ((IacaSet *) v2) : 0;
  IacaSet *newset = 0;
  IacaItem **bigtab = 0;
  unsigned card1 = 0, card2 = 0;
  unsigned newcard = 0;
  unsigned ix1 = 0, ix2 = 0;
  if (!set1 || !set2
      || (card1 = set1->v_cardinal) == 0 || (card2 = set2->v_cardinal) == 0)
    return (IacaSet *) &iaca_empty_setdata;
  if (card1 > card2)
    {
      IacaSet *setmp = set1;
      unsigned cardtmp = card1;
      set1 = set2;
      set2 = setmp;
      card1 = card2;
      card2 = cardtmp;
    };
  newset = iaca_alloc_data (sizeof (IacaSet) + card1 * sizeof (IacaItem *));
  bigtab = newset->v_elements;
  while (ix1 < card1 && ix2 < card2)
    {
      IacaItem *it1 = set1->v_elements[ix1];
      IacaItem *it2 = set2->v_elements[ix2];
      if (it1 == it2)
	{
	  bigtab[newcard++] = it1;
	  ix1++;
	  ix2++;
	  continue;
	}
      else if (it1->v_ident < it2->v_ident)
	ix1++;
      else
	ix2++;
    }
  if (newcard + 1 < 7 * card1 / 8)
    {
      IacaSet *bigset = newset;
      newset =
	iaca_alloc_data (sizeof (IacaSet) + newcard * sizeof (IacaItem *));
      for (unsigned ix = 0; ix < newcard; ix++)
	newset->v_elements[ix] = bigset->v_elements[ix];
      GC_FREE (bigset);
    };
  newset->v_cardinal = newcard;
  newset->v_kind = IACAV_SET;
  return newset;
}


IacaSet *
iaca_set_difference (IacaValue *v1, IacaValue *v2)
{
  unsigned ix1 = 0, ix2 = 0, card1 = 0, card2 = 0;
  unsigned newcard = 0;
  IacaSet *newset = 0;
  IacaSet *set1 = (v1 && v1->v_kind == IACAV_SET) ? ((IacaSet *) v1) : 0;
  IacaSet *set2 = (v2 && v2->v_kind == IACAV_SET) ? ((IacaSet *) v2) : 0;
  if (!set1 || (card1 = set1->v_cardinal) == 0)
    return (IacaSet *) &iaca_empty_setdata;
  if (!set2 || (card2 = set2->v_cardinal) == 0)
    return set1;
  if (card2 == 1 && !iaca_set_contains ((IacaValue *) set1,
					(IacaValue *) (set2->v_elements[0])))
    return set1;
  newset = iaca_alloc_data (sizeof (IacaSet) + card1 * sizeof (IacaItem *));
  while (ix1 < card1 && ix2 < card2)
    {
      IacaItem *it1 = set1->v_elements[ix1];
      IacaItem *it2 = set2->v_elements[ix2];
      if (it1 == it2)
	{
	  ix1++;
	  ix2++;
	  continue;
	}
      if (it1->v_ident < it2->v_ident)
	{
	  newset->v_elements[newcard++] = it1;
	  ix1++;
	}
      else
	ix2++;
    }
  if (newcard + 1 < 7 * card1 / 8)
    {
      IacaSet *bigset = newset;
      newset =
	iaca_alloc_data (sizeof (IacaSet) + newcard * sizeof (IacaItem *));
      for (unsigned ix = 0; ix < newcard; ix++)
	newset->v_elements[ix] = bigset->v_elements[ix];
      GC_FREE (bigset);
    };
  newset->v_cardinal = newcard;
  newset->v_kind = IACAV_SET;
  return newset;
}


IacaSet *
iaca_set_symetric_difference (IacaValue *v1, IacaValue *v2)
{
  unsigned ix1 = 0, ix2 = 0, card1 = 0, card2 = 0;
  unsigned newcard = 0, newsiz = 0;
  IacaSet *newset = 0;
  IacaSet *set1 = (v1 && v1->v_kind == IACAV_SET) ? ((IacaSet *) v1) : 0;
  IacaSet *set2 = (v2 && v2->v_kind == IACAV_SET) ? ((IacaSet *) v2) : 0;
  if (!set1 || (card1 = set1->v_cardinal) == 0)
    return set2;
  if (!set2 || (card2 = set2->v_cardinal) == 0)
    return set1;
  if (card2 == 1 && !iaca_set_contains ((IacaValue *) set1,
					(IacaValue *) (set2->v_elements[0])))
    return set1;
  if (card1 == 1 && !iaca_set_contains ((IacaValue *) set2,
					(IacaValue *) (set1->v_elements[0])))
    return set2;
  newsiz = card1 + card2;
  newset = iaca_alloc_data (sizeof (IacaSet) + newsiz * sizeof (IacaItem *));
  while (ix1 < card1 && ix2 < card2)
    {
      IacaItem *it1 = set1->v_elements[ix1];
      IacaItem *it2 = set2->v_elements[ix2];
      if (it1 == it2)
	{
	  ix1++;
	  ix2++;
	  continue;
	}
      if (it1->v_ident < it2->v_ident)
	{
	  newset->v_elements[newcard++] = it1;
	  ix1++;
	}
      else if (it1->v_ident > it2->v_ident)
	{
	  newset->v_elements[newcard++] = it2;
	  ix2++;
	}
    }
  if (newcard + 1 < 7 * newsiz / 8)
    {
      IacaSet *bigset = newset;
      newset =
	iaca_alloc_data (sizeof (IacaSet) + newcard * sizeof (IacaItem *));
      for (unsigned ix = 0; ix < newcard; ix++)
	newset->v_elements[ix] = bigset->v_elements[ix];
      GC_FREE (bigset);
    };
  newset->v_cardinal = newcard;
  newset->v_kind = IACAV_SET;
  return newset;
}

////////////////////////////////////////////////////////////////
IacaItem *
iaca_item_make (struct iacadataspace_st *sp)
{
  IacaItem *itm = 0;
  g_assert (sp == NULL || sp->dsp_magic == IACA_SPACE_MAGIC);
  iaca.ia_item_last_ident++;
  itm = iaca_alloc_data (sizeof (IacaItem));
  itm->v_kind = IACAV_ITEM;
  itm->v_ident = iaca.ia_item_last_ident;
  itm->v_attrtab = NULL;
  itm->v_payloadkind = IACAPAYLOAD__NONE;
  itm->v_payloadptr = NULL;
  itm->v_itemcontent = NULL;
  itm->v_dataspace = sp;
  return itm;
}



/* internal UNSAFE routine putting an attribute inside a table, and
   returning the index where it has been put or else -1. Don't call it
   if tbl is not a table or itat not an item or val not a non-null
   value! */
static inline int
iaca_attribute_put_index_unsafe (struct iacatabattr_st *tbl, IacaItem *itat,
				 IacaValue *val)
{
  int pos = -1;			/* insert position */
  unsigned sz = tbl->at_size;
  unsigned h = itat->v_ident % sz;
  for (unsigned ix = h; ix < sz; ix++)
    {
      IacaItem *curit = tbl->at_entab[ix].en_item;
      if (curit == itat)
	{
	  tbl->at_entab[ix].en_val = val;
	  return (int) ix;
	}
      else if (curit == IACA_EMPTY_SLOT)	/* emptied slot */
	{
	  if (pos < 0)		/* register the available position */
	    pos = ix;
	  continue;
	}
      else if (!curit)
	{
	  if (pos < 0)		/* register the available position */
	    pos = ix;
	  break;
	}
    }
  if (pos >= 0)
    {
      tbl->at_entab[pos].en_item = itat;
      tbl->at_entab[pos].en_val = val;
      tbl->at_count++;
      return pos;
    }
  for (unsigned ix = 0; ix < h; ix++)
    {
      IacaItem *curit = tbl->at_entab[ix].en_item;
      if (curit == itat)
	{
	  tbl->at_entab[ix].en_val = val;
	  return (int) ix;
	}
      else if (curit == IACA_EMPTY_SLOT)	/* emptied slot */
	{
	  if (pos < 0)		/* register the available position */
	    pos = ix;
	  continue;
	}
      else if (!curit)
	{
	  if (pos < 0)		/* register the available position */
	    pos = ix;
	  break;
	}
    }
  if (pos >= 0)
    {
      tbl->at_entab[pos].en_item = itat;
      tbl->at_entab[pos].en_val = val;
      tbl->at_count++;
      return pos;
    }
  /* this can also happen if the table is completely full */
  return -1;
}

void
iaca_item_attribute_reorganize (IacaValue *vitem, unsigned xtra)
{
  IacaItem *item = 0;
  struct iacatabattr_st *tbl = 0;
  struct iacatabattr_st *newtbl = 0;
  unsigned sz = 0;
  unsigned newsz = 0;
  unsigned cnt = 0;
  if (!vitem || vitem->v_kind != IACAV_ITEM)
    return;
  item = (IacaItem *) vitem;
  tbl = item->v_attrtab;
  if (!tbl)
    return;
  sz = tbl->at_size;
  cnt = tbl->at_count;
  if (sz <= 3 && cnt + xtra <= sz)
    return;
  if (cnt + xtra < sz / 4)
    /* should shrink the table */
    newsz = (cnt + xtra) / 2 + 1;
  else if (5 * (cnt + xtra) + 1 > 4 * sz)
    /* should grow the table */
    newsz = (cnt + xtra) / 2 + 1;
  else
    return;
  /* round up newsz to a prime */
  if (newsz <= 3)
    newsz = 3;
  else if (newsz <= 7)
    newsz = 7;
  else if (newsz <= 11)
    newsz = 11;
  else if (newsz <= 17)
    newsz = 17;
  else
    newsz = g_spaced_primes_closest (newsz);
  g_assert (newsz >= cnt / 2);
  if (newsz == sz)
    return;
  newtbl =
    (struct iacatabattr_st *) iaca_alloc_data (sizeof (struct iacatabattr_st)
					       +
					       newsz *
					       sizeof (struct
						       iacaentryattr_st));
  newtbl->at_size = newsz;
  newtbl->at_count = 0;
  item->v_attrtab = newtbl;
  for (unsigned ix = 0; ix < sz; ix++)
    {
      IacaItem *curat = tbl->at_entab[ix].en_item;
      int pos = -1;
      if (!curat || curat == IACA_EMPTY_SLOT)
	continue;
      pos =
	iaca_attribute_put_index_unsafe (newtbl, curat,
					 tbl->at_entab[ix].en_val);
      g_assert (pos >= 0);
    }
  g_assert (tbl->at_count == newtbl->at_count);
  /* we are sure the old tbl is not reachable and can be freed */
  GC_FREE (tbl);
}

void
iaca_item_physical_put (IacaValue *vitem, IacaValue *vattr, IacaValue *val)
{
  IacaItem *item = 0;
  IacaItem *attr = 0;
  struct iacatabattr_st *tbl = 0;
  int ix = -1;
  if (!vitem || vitem->v_kind != IACAV_ITEM
      || !vattr || vattr->v_kind != IACAV_ITEM || !val)
    return;
  item = (IacaItem *) vitem;
  attr = (IacaItem *) vattr;
  tbl = item->v_attrtab;
  if (!tbl)
    {
      unsigned sz = 3;
      tbl =
	(struct iacatabattr_st *)
	iaca_alloc_data (sizeof (struct iacatabattr_st) +
			 sz * sizeof (struct iacaentryattr_st));
      tbl->at_size = sz;
      tbl->at_count = 0;
      item->v_attrtab = tbl;
      ix = iaca_attribute_put_index_unsafe (tbl, attr, val);
      g_assert (ix >= 0);
      return;
    }
  if (5 * tbl->at_count > 4 * tbl->at_size)
    {
      iaca_item_attribute_reorganize (vitem, 1);
      item = (IacaItem *) vitem;
      tbl = item->v_attrtab;
    }
  ix = iaca_attribute_put_index_unsafe (tbl, attr, val);
  g_assert (ix >= 0);
}



IacaValue *
iaca_item_physical_remove (IacaValue *vitem, IacaValue *vattr)
{
  IacaValue *oldval = 0;
  IacaItem *item = 0;
  IacaItem *attr = 0;
  struct iacatabattr_st *tbl = 0;
  int ix = -1;
  if (!vitem || vitem->v_kind != IACAV_ITEM
      || !vattr || vattr->v_kind != IACAV_ITEM)
    return NULL;
  item = (IacaItem *) vitem;
  attr = (IacaItem *) vattr;
  tbl = item->v_attrtab;
  if (!tbl)
    return NULL;
  ix = iaca_attribute_index_unsafe (tbl, attr);
  if (ix < 0)
    return NULL;
  oldval = tbl->at_entab[ix].en_val;
  g_assert (oldval != 0);
  tbl->at_entab[ix].en_item = IACA_EMPTY_SLOT;
  tbl->at_entab[ix].en_val = 0;
  tbl->at_count--;
  if (tbl->at_size > 10 && tbl->at_count < tbl->at_size / 5)
    iaca_item_attribute_reorganize (vitem, 0);
  return oldval;
}


/* get a dataspace by its name; dataspaces are never freed */
struct iacadataspace_st *
iaca_dataspace (const char *name)
{
  struct iacadataspace_st *dsp = 0;
  if (!name || !name[0])
    return NULL;
  for (const char *pc = name; *pc; pc++)
    if (!g_ascii_isalnum (*pc) && *pc != '_')
      return NULL;
  if (!iaca.ia_dataspace_htab)
    iaca.ia_dataspace_htab = g_hash_table_new (g_str_hash, g_str_equal);
  dsp = g_hash_table_lookup (iaca.ia_dataspace_htab, name);
  if (!dsp)
    {
      IacaString *strn = iaca_string_make (name);
      dsp = g_malloc0 (sizeof (*dsp));
      dsp->dsp_name = strn;
      dsp->dsp_magic = IACA_SPACE_MAGIC;
      g_hash_table_insert (iaca.ia_dataspace_htab,
			   (gpointer) iaca_string_val ((IacaValue *) strn),
			   dsp);
    }
  g_assert (dsp->dsp_magic == IACA_SPACE_MAGIC);
  return dsp;
}

int
main (int argc, char **argv)
{
  GC_INIT ();
  gtk_init (&argc, &argv);
  /* we force GC friendship in GTK & GLIB! */
  g_mem_gc_friendly = TRUE;
#warning temporary load current directory
  iaca_load (".");
}
