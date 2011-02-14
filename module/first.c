// file module/first.c
#include "iaca.h"

void
iacamod_first_init1 (void)
{
  IacaItem *ita = 0, *itb = 0, *itc = 0, *itd = 0, *ite = 0, *itdict = 0;
  IacaValue *v1 = 0, *v2 = 0, *v3 = 0;
  struct iacadataspace_st *spf = iaca_dataspace ("firstspace");
  iaca_debug ("init1 of first spf=%p", spf);
  iaca.ia_topdictitm = itdict = iaca_item_make (spf);
  iaca_item_pay_load_reserve_dictionnary (itdict, 15);
  ita = iaca_item_make (spf);
  iaca_item_pay_load_put_dictionnary_str (itdict, "a", (IacaValue *) ita);
  itb = iaca_item_make (spf);
  iaca_item_pay_load_put_dictionnary_str (itdict, "b", (IacaValue *) itb);
  itc = iaca_item_make (spf);
  iaca_item_pay_load_put_dictionnary_str (itdict, "c", (IacaValue *) itc);
  itd = iaca_item_make (spf);
  iaca_item_pay_load_put_dictionnary_str (itdict, "d", (IacaValue *) itd);
  ite = iaca_item_make (spf);
  iaca_item_pay_load_put_dictionnary_str (itdict, "e", (IacaValue *) ite);
  iaca_item_pay_load_put_dictionnary_str (itdict, "thedict",
					  (IacaValue *) itdict);
  iaca_debug ("ita %p itb %p itc %p itd %p ite %p", ita, itb, itc, itd, ite);
  v1 = iacav_string_make ("some\nstring");
  iaca_item_physical_put ((IacaValue *) ita, (IacaValue *) itb, v1);
  v3 = iacav_node_makevar ((IacaValue *) itc, ita);
  v2 = iacav_node_makevar ((IacaValue *) itb, ita, iacav_integer_make (234),
			   v3);
  iaca_item_physical_put ((IacaValue *) ita, (IacaValue *) itc, v2);
  iaca_item_physical_put ((IacaValue *) ita, (IacaValue *) itd,
			  (IacaValue *) ite);
  iaca_item_physical_put ((IacaValue *) itc, (IacaValue *) ita,
			  (IacaValue *) itb);
  v3 = iacav_set_makenewvar (ita, itb, itd);
  iaca_item_physical_put ((IacaValue *) itb, (IacaValue *) ite, v3);
  iaca_dump ("/tmp/firstiaca");
}
