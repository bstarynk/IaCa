
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



void
iaca_item_pay_load_resize_vector (IacaItem *itm, unsigned len)
{
  unsigned sz = 0;
  struct iacapayloadvector_st *oldpv = 0;
  if (!itm || itm->v_kind != IACAV_ITEM)
    return;
  for (unsigned nsz = 8; nsz < UINT_MAX / 2; nsz = nsz * 2)
    {
      unsigned s = nsz - 2;
      if (s >= len)
	{
	  sz = s;
	  break;
	};
      s = 3 * nsz / 2 - 3;
      if (s > len)
	{
	  sz = s;
	  break;
	}
    }
  if (sz == 0)
    iaca_error ("too big vector length %u", len);
  if (itm->v_payloadkind != IACAPAYLOAD_VECTOR
      || (oldpv = itm->v_payloadvect) == 0)
    {
      struct iacapayloadvector_st *pv = 0;
      iaca_item_pay_load_clear (itm);
      pv =
	iaca_alloc_data (sizeof (struct iacapayloadvector_st) +
			 sz * sizeof (IacaValue *));
      pv->vec_siz = sz;
      pv->vec_len = len;
      itm->v_payloadkind = IACAPAYLOAD_VECTOR;
      itm->v_payloadvect = pv;
      return;
    }
  else if (oldpv->vec_siz < len)
    {
      struct iacapayloadvector_st *pv = 0;
      pv =
	iaca_alloc_data (sizeof (struct iacapayloadvector_st) +
			 sz * sizeof (IacaValue *));
      pv->vec_siz = sz;
      pv->vec_len = len;
      memcpy (pv->vec_tab, oldpv->vec_tab,
	      sizeof (struct iacapayloadvector_st) +
	      (oldpv->vec_len) * sizeof (IacaValue *));
      itm->v_payloadvect = pv;
      GC_FREE (oldpv);
    }
  else if (oldpv->vec_siz > sz)
    {
      struct iacapayloadvector_st *pv = 0;
      pv =
	iaca_alloc_data (sizeof (struct iacapayloadvector_st) +
			 sz * sizeof (IacaValue *));
      pv->vec_siz = sz;
      pv->vec_len = len;
      memcpy (pv->vec_tab, oldpv->vec_tab,
	      sizeof (struct iacapayloadvector_st) +
	      len * sizeof (IacaValue *));
      itm->v_payloadvect = pv;
      GC_FREE (oldpv);
    }
  else
    {
      memset (oldpv->vec_tab + len, 0,
	      sizeof (void *) * (oldpv->vec_siz - len));
      oldpv->vec_len = len;
    }
}


void
iaca_item_pay_load_append_vector (IacaItem *itm, IacaValue *val)
{
  struct iacapayloadvector_st *oldpv = 0;
  if (!itm || itm->v_kind != IACAV_ITEM)
    return;
  if (itm->v_payloadkind != IACAPAYLOAD_VECTOR)
    return;
  if ((oldpv = itm->v_payloadvect) == 0)
    {
      unsigned sz = 7;
      struct iacapayloadvector_st *pv = 0;
      iaca_item_pay_load_clear (itm);
      pv =
	iaca_alloc_data (sizeof (struct iacapayloadvector_st) +
			 sz * sizeof (IacaValue *));
      pv->vec_siz = sz;
      pv->vec_len = 1;
      pv->vec_tab[0] = val;
      itm->v_payloadkind = IACAPAYLOAD_VECTOR;
      itm->v_payloadvect = pv;
      return;
    }
  else if (oldpv->vec_len + 2 >= oldpv->vec_siz)
    {
      iaca_item_pay_load_resize_vector (itm,
					oldpv->vec_len + 2 +
					oldpv->vec_len / 8);
      oldpv = itm->v_payloadvect;
    }
  oldpv->vec_tab[oldpv->vec_len++] = val;
}


void
iaca_item_pay_load_reserve_buffer (IacaItem *itm, unsigned sz)
{
  struct iacapayloadbuffer_st *oldbuf = 0;
  unsigned nsz = 0;
  if (!itm || itm->v_kind != IACAV_ITEM)
    return;
  for (unsigned z = 8 * sizeof (struct iacapayloadbuffer_st); z > 0;
       z = 2 * z)
    {
      unsigned s = z - 2 * sizeof (struct iacapayloadbuffer_st);
      if (s > sz)
	{
	  nsz = s;
	  break;
	}
      s = 3 * sz / 2 - 2 * sizeof (struct iacapayloadbuffer_st);
      if (s > sz)
	{
	  nsz = s;
	  break;
	};
    }
  if (nsz == 0)
    iaca_error ("too big buffer size %u", sz);
  if (itm->v_payloadkind != IACAPAYLOAD_BUFFER)
    {
      struct iacapayloadbuffer_st *buf = 0;
      iaca_item_pay_load_clear (itm);
      buf = iaca_alloc_atomic (sizeof (buf) + nsz);
      buf->buf_siz = nsz;
      buf->buf_len = 0;
      itm->v_payloadkind = IACAPAYLOAD_BUFFER;
      itm->v_payloadbuf = buf;
      return;
    }
  oldbuf = itm->v_payloadbuf;
  if (!oldbuf || oldbuf->buf_siz < nsz)
    {
      struct iacapayloadbuffer_st *buf = 0;
      buf = iaca_alloc_atomic (sizeof (buf) + nsz);
      if (oldbuf)
	memcpy (buf->buf_tab, oldbuf->buf_tab, oldbuf->buf_len);
      buf->buf_siz = nsz;
      buf->buf_len = oldbuf ? oldbuf->buf_len : 0;
      itm->v_payloadbuf = buf;
      if (oldbuf)
	GC_FREE (oldbuf);
    }
}


void
iaca_item_pay_load_append_buffer (IacaItem *itm, const char *str)
{
  int slen = 0;
  unsigned oldlen = 0;
  struct iacapayloadbuffer_st *oldbuf = 0;
  if (!itm || itm->v_kind != IACAV_ITEM || !str
      || itm->v_payloadkind != IACAPAYLOAD_BUFFER)
    return;
  slen = strlen (str);
  oldbuf = itm->v_payloadbuf;
  oldlen = oldbuf ? oldbuf->buf_len : 0;
  if (!oldbuf || oldbuf->buf_siz < slen + oldlen + 2)
    {
      iaca_item_pay_load_reserve_buffer (itm,
					 slen + oldlen + oldlen / 16 +
					 slen / 8 + 10);
      oldbuf = itm->v_payloadbuf;
    }
  memcpy (oldbuf->buf_tab + oldlen, str, slen);
  oldbuf->buf_len = oldlen + slen;
}


void
iaca_item_pay_load_append_cencoded_buffer (IacaItem *itm, const char *str)
{
  if (!itm || itm->v_kind != IACAV_ITEM || !str
      || itm->v_payloadkind != IACAPAYLOAD_BUFFER)
    return;
  for (const char *pc = str; *pc; pc++)
    {
      char c = *pc;
      switch (c)
	{
	case '\\':
	  iaca_item_pay_load_append_buffer (itm, "\\\\");
	  break;
	case '\'':
	  iaca_item_pay_load_append_buffer (itm, "\\\'");
	  break;
	case '\"':
	  iaca_item_pay_load_append_buffer (itm, "\\\"");
	  break;
	case '\r':
	  iaca_item_pay_load_append_buffer (itm, "\\r");
	  break;
	case '\n':
	  iaca_item_pay_load_append_buffer (itm, "\\n");
	  break;
	case '\t':
	  iaca_item_pay_load_append_buffer (itm, "\\t");
	  break;
	case '\v':
	  iaca_item_pay_load_append_buffer (itm, "\\v");
	  break;
	case '\f':
	  iaca_item_pay_load_append_buffer (itm, "\\f");
	  break;
	case 'a' ... 'z':
	case 'A' ... 'Z':
	case '0' ... '9':
	case ' ':
	case '-':
	case '_':
	  {
	    char cbuf[2];
	    cbuf[0] = c;
	    cbuf[1] = 0;
	    iaca_item_pay_load_append_buffer (itm, cbuf);
	    break;
	  }
	default:
	  if (c > ' ' && isprint (c))
	    {
	      char cbuf[2];
	      cbuf[0] = c;
	      cbuf[1] = 0;
	      iaca_item_pay_load_append_buffer (itm, cbuf);
	    }
	  else
	    {
	      char cbuf[8];
	      memset (cbuf, 0, sizeof (cbuf));
	      snprintf (cbuf, sizeof (cbuf) - 1, "\\x%02x", c & 0xff);
	      iaca_item_pay_load_append_buffer (itm, cbuf);
	    }
	}
    }
}


void
iaca_item_pay_load_bufprintf (IacaItem *itm, const char *fmt, ...)
{
  int fmtlen = strlen (fmt);
  unsigned len = 0;
  unsigned siz = 0;
  int pl = 0;
  struct iacapayloadbuffer_st *buf = 0;
  va_list args;
  if (!itm || itm->v_kind != IACAV_ITEM || !fmt
      || itm->v_payloadkind != IACAPAYLOAD_BUFFER)
    return;
  buf = itm->v_payloadbuf;
  len = buf ? buf->buf_len : 0;
  siz = buf ? buf->buf_siz : 0;
  if (!buf || len + 4 * fmtlen + 32 > siz)
    {
      iaca_item_pay_load_reserve_buffer (itm, len + 5 * fmtlen + 64);
      buf = itm->v_payloadbuf;
      len = buf ? buf->buf_len : 0;
      siz = buf ? buf->buf_siz : 0;
    }
  va_start (args, fmt);
  pl = vsnprintf (buf->buf_tab + len, siz - len - 3, fmt, args);
  va_end (args);
  if (len + pl > siz - 3)
    {
      iaca_item_pay_load_reserve_buffer (itm, len + pl + fmtlen + 32);
      buf = itm->v_payloadbuf;
      len = buf ? buf->buf_len : 0;
      siz = buf ? buf->buf_siz : 0;
      va_start (args, fmt);
      pl = vsnprintf (buf->buf_tab + len, siz - len - 3, fmt, args);
      va_end (args);
    }
  buf->buf_len += pl;
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
