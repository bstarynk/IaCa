// file module/first.c
/****
  © 2011  Basile Starynkevitch

    this file first.c is part of IaCa

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


static struct iacadataspace_st *iacafirst_dsp;

/// initialize the application
enum iacagtkinitval_en
{
  IACAGTKINITVAL_ACTIVEAPPL,
  IACAGTKINITVAL__LAST
};

static void
iacafirst_gtkinit (GObject *gob, IacaItem *cloitm)
{
  GtkApplication *gapp = GTK_APPLICATION (gob);
  IacaItem *itactivapp = iacac_item (iaca_item_pay_load_closure_nth (cloitm,
								     IACAGTKINITVAL_ACTIVEAPPL));
  iaca_debug ("gapp %p itactivapp %p", gapp, itactivapp);
  g_assert (GTK_IS_APPLICATION (gapp));
  g_signal_connect ((GObject *) gapp, "activate",
		    G_CALLBACK (iaca_item_pay_load_closure_gobject_do),
		    (gpointer) itactivapp);
}

IACA_DEFINE_CLOFUN (gtkapplinit,
		    IACAGTKINITVAL__LAST, gobject_do, iacafirst_gtkinit);

////////////////////////////////////////////////////////////////

/// activate the application
enum iacaactivateapplicationval_en
{
  IACAACTIVATEAPPLICATIONVAL__LAST
};
static void
iacafirst_activateapplication (GObject *gapp, IacaItem *cloitm)
{
  GtkApplication *app = GTK_APPLICATION (gapp);
  iaca_debug ("gapp %p cloitm %p #%lld", gapp, cloitm, iaca_item_identll(cloitm));
  g_assert (GTK_IS_APPLICATION(app));
}

IACA_DEFINE_CLOFUN (activateapplication,
		    IACAACTIVATEAPPLICATIONVAL__LAST,
		    gobject_do, iacafirst_activateapplication);

////////////////////////////////////////////////////////////////

void
iacamod_first_init1 (void)
{
  IacaItem *itdict = 0;
  iacafirst_dsp = iaca_dataspace ("firstspace");
  iaca_debug ("init1 of first iacafirst_dsp=%p", iacafirst_dsp);
  if (!(itdict = iaca.ia_topdictitm))
    iaca_error ("missing top level dictionnary");
  iaca_debug ("iaca.ia_gtkinititm %p #%lld", 
	      iaca.ia_gtkinititm, iaca_item_identll (iaca.ia_gtkinititm));
  {
    IacaItem* itapp = 0;
    itapp =  iaca_item_pay_load_closure_nth (iaca.ia_gtkinititm,
					     IACAGTKINITVAL_ACTIVEAPPL);
    if (!itapp)
      itapp =  iaca_item_make (iacafirst_dsp);
    iaca_item_pay_load_make_closure (itapp,
				     &iacacfun_activateapplication,
				     (IacaValue **) 0);
    iaca_item_pay_load_closure_set_nth (iaca.ia_gtkinititm,
					IACAGTKINITVAL_ACTIVEAPPL,
					(IacaValue*)itapp);
  }
#if 0
  {
    itvaluedisplayer = iaca_item_make (iacafirst_dsp);
    iaca_item_pay_load_make_closure (itvaluedisplayer,
				     &iacacfun_valuedisplayer,
				     (IacaValue **) 0);
    iaca_item_pay_load_closure_set_nth
      (itvaluedisplayer,
       IACAVALUEDISPLAYER_DISPLAYITEMREF, (IacaValue *) ititrefdisplayer);
  }
#endif
}
