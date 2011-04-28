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
static GtkApplication *iacafirst_gapp;

/* the boxed gobject value for the top entry */
static IacaValue *iacafirst_valentry;

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
  iacafirst_gapp = gapp;
  g_signal_connect ((GObject *) gapp, "activate",
		    G_CALLBACK (iaca_item_pay_load_closure_gobject_do),
		    (gpointer) itactivapp);
}

IACA_DEFINE_CLOFUN (gtkapplinit,
		    IACAGTKINITVAL__LAST, gobject_do, iacafirst_gtkinit);

////////////////////////////////////////////////////////////////

/// closed values for item editor
enum iacaitemeditorval_en
{
  IACAITEMEDITOR_SIMPLE_EDITING,
  IACAITEMEDITOR__LAST
};


/* should return a boxed window */
static IacaValue *
iacafirst_itemeditor (IacaValue *v1, IacaItem *cloitm)
{
  IacaValue *res = NULL;
  IacaItem *nitm = iacac_item (v1);
  GtkWindow *winedit = NULL;
  iaca_debug ("start v1 %p nitm#%lld cloitm %p",
	      v1, iaca_item_identll (nitm), cloitm);
  if (!nitm)
    return NULL;
  winedit = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  gtk_window_set_application (winedit, iacafirst_gapp);
  return res;
}

IACA_DEFINE_CLOFUN (itemeditor,
		    IACAITEMEDITOR__LAST, one_value, iacafirst_itemeditor);

////////////////////////////////////////////////////////////////

static void
edit_named_item (const char *txt, IacaItem *nameditm, IacaItem *itemeditoritm)
{
  IacaValue *vedit = NULL;
  iaca_debug ("txt '%s' nameditm %p #%lld itemeditoritm %p #%lld",
	      txt, nameditm, iaca_item_identll (nameditm),
	      itemeditoritm, iaca_item_identll (itemeditoritm));
  vedit =
    iaca_item_pay_load_closure_one_value ((IacaValue *) nameditm,
					  itemeditoritm);
  iaca_debug ("vedit %p", vedit);
}

static void
bad_entry_name_dialog (const char *txt)
{
  GtkWidget *dial = 0;
  GtkEntry *entry = GTK_ENTRY (iaca_gobject (iacafirst_valentry));
  int resp = 0;
  dial = gtk_message_dialog_new_with_markup
    ((GtkWindow *) 0,
     GTK_DIALOG_DESTROY_WITH_PARENT,
     GTK_MESSAGE_WARNING,
     GTK_BUTTONS_OK, "<b>invalid name</b> <tt>%s</tt>", txt);
  gtk_message_dialog_format_secondary_markup
    (GTK_MESSAGE_DIALOG (dial),
     "A name should contain only <i>letters</i> or underscores <tt>_</tt>");
  gtk_widget_show_all (GTK_WIDGET (dial));
  resp = gtk_dialog_run (GTK_DIALOG (dial));
  iaca_debug ("resp %d", resp);
  gtk_widget_destroy (GTK_WIDGET (dial)), dial = 0;
  gtk_entry_set_text (entry, "");
}

static void
make_new_item_dialog (const char *txt, IacaItem *itemeditoritm)
{
  GtkWidget *dial = 0;
  IacaItem *nameditm = 0;
  int resp = 0;
  dial = gtk_message_dialog_new_with_markup
    ((GtkWindow *) 0,
     GTK_DIALOG_DESTROY_WITH_PARENT,
     GTK_MESSAGE_QUESTION,
     GTK_BUTTONS_OK_CANCEL,
     "<b>Create new item</b> named <tt>%s</tt> ?", txt);
  gtk_message_dialog_format_secondary_markup
    (GTK_MESSAGE_DIALOG (dial),
     "<i>ok</i> to create then edit a new named item,\n"
     "<i>cancel</i> to continue without changes.");
  gtk_widget_show_all (GTK_WIDGET (dial));
  resp = gtk_dialog_run (GTK_DIALOG (dial));
  iaca_debug ("resp %d", resp);
  if (resp == GTK_RESPONSE_OK)
    {
      nameditm = iaca_item_make (iacafirst_dsp);
      iaca_debug
	("before put dict str topdictitm %p #%lld txt '%s' nameditm %p #%lld",
	 iaca.ia_topdictitm, iaca_item_identll (iaca.ia_topdictitm), txt,
	 nameditm, iaca_item_identll (nameditm));
      iaca_item_pay_load_put_dictionnary_str (iaca.ia_topdictitm, txt,
					      (IacaValue *) nameditm);
      iaca_debug ("created named '%s' %p #%lld", txt, nameditm,
		  iaca_item_identll (nameditm));
      edit_named_item (txt, nameditm, itemeditoritm);
    }
  else
    nameditm = 0;
  gtk_widget_destroy (GTK_WIDGET (dial)), dial = 0;
}

static void
popup_final_dialog (GtkWidget *w, gpointer ptr)
{
  GtkWidget *dial = 0;
  GtkWidget *lab = 0;
  char *markup = 0;
  char *realstatedir = 0;
  guint res = 0;
  g_assert (ptr == NULL);
  g_assert (w != NULL);
  dial = gtk_dialog_new_with_buttons ("Finally dump state",
				      (GtkWindow *) 0,
				      GTK_DIALOG_MODAL |
				      GTK_DIALOG_DESTROY_WITH_PARENT,
				      GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
				      NULL);
  iaca_debug ("dial %p", dial);
  lab = gtk_label_new (NULL);
  realstatedir = realpath (iaca.ia_statedir, NULL);
  markup =
    g_markup_printf_escaped ("<i>Ok</i> to save Iaca state in <tt>%s</tt>\n"
			     "<i>Cancel</i> to quit Iaca without saving",
			     realstatedir);
  gtk_label_set_markup (GTK_LABEL (lab), markup);
  free (realstatedir), realstatedir = 0;
  g_free (markup), markup = 0;
  gtk_container_add (GTK_CONTAINER
		     (gtk_dialog_get_content_area (GTK_DIALOG (dial))), lab);
  gtk_widget_show_all (dial);
  res = gtk_dialog_run (GTK_DIALOG (dial));
  iaca_debug ("res %u", res);
  if (res == GTK_RESPONSE_ACCEPT)
    {
      iaca_debug ("accept dumping state to %s", iaca.ia_statedir);
      iaca_dump (iaca.ia_statedir);
    }
  else
    {
      iaca_debug ("dont dump state but quit");
    }
  gtk_widget_destroy (GTK_WIDGET (dial)), dial = 0;
  gtk_main_quit ();
}

static void
save_dialog_cb (GtkWidget *w, gpointer ptr)
{
  GtkWidget *dial = 0;
  gint resp = 0;
  iaca_debug ("begin");
  dial = gtk_message_dialog_new_with_markup
    ((GtkWindow *) 0,
     GTK_DIALOG_DESTROY_WITH_PARENT,
     GTK_MESSAGE_QUESTION,
     GTK_BUTTONS_NONE,
     "<b>Save IaCa state</b> to directory\n <tt>%s</tt> ?\n",
     iaca.ia_statedir);
  gtk_dialog_add_buttons (GTK_DIALOG (dial),
			  GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
			  GTK_STOCK_CLOSE, GTK_RESPONSE_NO,
			  GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
  gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dial),
					      "<i>Save</i> to save the state to directory\n <tt>%s</tt> and continue,\n\n"
					      "<i>Close</i> to save the state and quit,\n\n"
					      "<i>Cancel</i> to continue without saving\n",
					      iaca.ia_statedir);
  gtk_widget_show_all (GTK_WIDGET (dial));
  resp = gtk_dialog_run (GTK_DIALOG (dial));
  iaca_debug ("resp %d", resp);
  switch (resp)
    {
    case GTK_RESPONSE_ACCEPT:
      /* save and continue */
      iaca_debug ("save to %s and continue", iaca.ia_statedir);
      iaca_dump (iaca.ia_statedir);
      break;
    case GTK_RESPONSE_NO:
      /* save and quit */
      iaca_debug ("save to %s and quit", iaca.ia_statedir);
      iaca_dump (iaca.ia_statedir);
      gtk_main_quit ();
      break;
    default:
      /* continue without saving */
      iaca_debug ("continue without saving");
      break;
    }
  gtk_widget_destroy (GTK_WIDGET (dial)), dial = 0;
}

static void
saveas_dialog_cb (GtkWidget *w, gpointer ptr)
{
  GtkWidget *dial = 0;
  gint resp = 0;
  iaca_debug ("begin");
  dial = gtk_file_chooser_dialog_new ("Save Iaca state to...",
				      (GtkWindow *) 0,
				      GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL);
  gtk_widget_show_all (GTK_WIDGET (dial));
  resp = gtk_dialog_run (GTK_DIALOG (dial));
  iaca_debug ("resp %d", resp);
  if (resp == GTK_RESPONSE_ACCEPT)
    {
      gchar *fildir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dial));
      iaca_debug ("fildir %s", fildir);
      iaca_dump (fildir);
      g_free (fildir);
    }
  gtk_widget_destroy (GTK_WIDGET (dial)), dial = 0;
}


static void
quit_dialog_cb (GtkWidget *w, gpointer ptr)
{
  GtkWidget *dial = 0;
  gint resp = 0;
  iaca_debug ("begin");
  dial = gtk_message_dialog_new_with_markup
    ((GtkWindow *) 0,
     GTK_DIALOG_DESTROY_WITH_PARENT,
     GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
     "<b>Quit IaCa without saving</b> state ?");
  gtk_dialog_add_buttons (GTK_DIALOG (dial), GTK_STOCK_QUIT, GTK_RESPONSE_NO,
			  GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
  gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dial),
					      "<i>Quit</i> without saving the state\n to <tt>%s</tt>,\n"
					      "<i>Cancel</i> to continue",
					      iaca.ia_statedir);
  gtk_widget_show_all (GTK_WIDGET (dial));
  resp = gtk_dialog_run (GTK_DIALOG (dial));
  iaca_debug ("resp %d", resp);
  switch (resp)
    {
    case GTK_RESPONSE_NO:
      /* quit */
      iaca_debug ("quit without saving");
      gtk_main_quit ();
      break;
    default:
      /* continue without saving */
      iaca_debug ("continue without saving");
      break;
    }
  gtk_widget_destroy (GTK_WIDGET (dial)), dial = 0;
}

static void
edit_named_cb (GtkWidget *menu, gpointer data)
{
  GtkEntry *entry = GTK_ENTRY (iaca_gobject (iacafirst_valentry));
  IacaItem *itemeditoritm = data;
  const gchar *txt = 0;
  const gchar *endtxt = 0;
  IacaItem *nameditm = 0;
  bool goodname = 0;
  g_assert (GTK_IS_ENTRY (entry));
  g_assert (itemeditoritm && itemeditoritm->v_kind == IACAV_ITEM);
  txt = gtk_entry_get_text (entry);
  iaca_debug ("txt '%s'", txt);
  if (!g_utf8_validate (txt, -1, &endtxt))
    iaca_error ("invalid UTF8 entry text '%s'", txt);
  goodname = endtxt > txt;
  for (const gchar *p = txt; p < endtxt && goodname; p = g_utf8_next_char (p))
    {
      gunichar c = g_utf8_get_char (p);
      goodname = g_unichar_isalpha (c) || c == '_';
    }
  iaca_debug ("text is %s", goodname ? "good name" : "bad");
  if (!goodname)
    {
      bad_entry_name_dialog (txt);
      return;
    }
  nameditm = iacac_item (iaca_item_pay_load_dictionnary_get
			 (iaca.ia_topdictitm, txt));
  iaca_debug ("nameditm %p #%lld", nameditm, iaca_item_identll (nameditm));
  if (!nameditm)
    make_new_item_dialog (txt, itemeditoritm);
  else
    edit_named_item (txt, nameditm, itemeditoritm);
}				/* end edit_named_cb */



/// activate the application
enum iacaactivateapplicationval_en
{
  IACAACTIVATEAPPLICATIONVAL_ITEM_EDITOR,
  IACAACTIVATEAPPLICATIONVAL__LAST
};
static void
iacafirst_activateapplication (GObject *gapp, IacaItem *cloitm)
{
  GtkWindow *win = 0;
  GtkWidget *box = 0;
  GtkWidget *ent = 0;
  IacaItem *itemeditoritm = 0;
  GtkWidget *menubar = 0;
  GtkWidget *filemenu = 0;
  GtkWidget *filesubmenu = 0;
  GtkWidget *savemenu = 0;
  GtkWidget *saveasmenu = 0;
  GtkWidget *quitmenu = 0;
  GtkWidget *editmenu = 0;
  GtkWidget *copymenu = 0;
  GtkWidget *cutmenu = 0;
  GtkWidget *namedmenu = 0;
  GtkWidget *pastemenu = 0;
  GtkWidget *editsubmenu = 0;
  GtkApplication *app = GTK_APPLICATION (gapp);
  iaca_debug ("gapp %p cloitm %p #%lld", gapp, cloitm,
	      iaca_item_identll (cloitm));
  g_assert (GTK_IS_APPLICATION (app));
  itemeditoritm =
    iacac_item
    (iaca_item_pay_load_closure_nth (cloitm,
				     IACAACTIVATEAPPLICATIONVAL_ITEM_EDITOR));
  win = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  g_signal_connect (win, "destroy", G_CALLBACK (popup_final_dialog),
		    (gpointer) 0);
  gtk_window_set_application (win, app);
  gtk_window_set_title (win, "iaca first");
  //  gtk_window_set_default_size (win, 580, 400);
  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
  /// create the menubar
  menubar = gtk_menu_bar_new ();
  filemenu = gtk_menu_item_new_with_mnemonic ("_File");
  filesubmenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (filemenu), filesubmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), filemenu);
  savemenu = gtk_menu_item_new_with_mnemonic ("_Save");
  saveasmenu = gtk_menu_item_new_with_mnemonic ("Save _As");
  quitmenu = gtk_menu_item_new_with_mnemonic ("_Quit");
  g_signal_connect (savemenu, "activate", G_CALLBACK (save_dialog_cb), NULL);
  g_signal_connect (saveasmenu, "activate", G_CALLBACK (saveas_dialog_cb),
		    NULL);
  g_signal_connect (quitmenu, "activate", G_CALLBACK (quit_dialog_cb), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (filesubmenu), savemenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (filesubmenu), saveasmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (filesubmenu), quitmenu);
  editmenu = gtk_menu_item_new_with_mnemonic ("Edit");
  editsubmenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (editmenu), editsubmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), editmenu);
  namedmenu = gtk_menu_item_new_with_mnemonic ("_Named");
  copymenu = gtk_menu_item_new_with_mnemonic ("_Copy");
  cutmenu = gtk_menu_item_new_with_mnemonic ("C_ut");
  pastemenu = gtk_menu_item_new_with_mnemonic ("_Paste");
  gtk_menu_shell_append (GTK_MENU_SHELL (editsubmenu), namedmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (editsubmenu), copymenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (editsubmenu), cutmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (editsubmenu), pastemenu);
  g_signal_connect (namedmenu, "activate", G_CALLBACK (edit_named_cb),
		    itemeditoritm);
  gtk_box_pack_start (GTK_BOX (box), menubar, FALSE, FALSE, 2);
  ent = gtk_entry_new ();
  iacafirst_valentry = iacav_gobject_box (G_OBJECT (ent));
  gtk_box_pack_start (GTK_BOX (box), ent, TRUE, TRUE, 2);
  gtk_container_add (GTK_CONTAINER (win), box);
  gtk_widget_show_all (GTK_WIDGET (win));
}

IACA_DEFINE_CLOFUN (activateapplication,
		    IACAACTIVATEAPPLICATIONVAL__LAST,
		    gobject_do, iacafirst_activateapplication);

////////////////////////////////////////////////////////////////

void
iacamod_first_init1 (void)
{
  IacaItem *itdict = NULL;
  IacaItem *itapp = NULL;
  IacaItem *ititemeditor = NULL;
  IacaItem *itsimpleediting = NULL;
  iacafirst_dsp = iaca_dataspace ("firstspace");
  iaca_debug ("init1 of first iacafirst_dsp=%p", iacafirst_dsp);
  if (!(itdict = iaca.ia_topdictitm))
    iaca_error ("missing top level dictionnary");
  iaca_debug ("iaca.ia_gtkinititm %p #%lld",
	      iaca.ia_gtkinititm, iaca_item_identll (iaca.ia_gtkinititm));
  itapp =
    iacac_item (iaca_item_pay_load_closure_nth (iaca.ia_gtkinititm,
						IACAGTKINITVAL_ACTIVEAPPL));
  iaca_debug ("itapp %p #%lld", itapp, iaca_item_identll (itapp));
  g_assert (itapp != NULL);
  ititemeditor =
    iacac_item (iaca_item_pay_load_closure_nth (itapp,
						IACAACTIVATEAPPLICATIONVAL_ITEM_EDITOR));
  iaca_debug ("ititemeditor %p #%lld", ititemeditor,
	      iaca_item_identll (ititemeditor));
  g_assert (ititemeditor != NULL);
  itsimpleediting =
    iacac_item (iaca_item_pay_load_closure_nth (ititemeditor,
						IACAITEMEDITOR_SIMPLE_EDITING));
  iaca_debug ("itsimpleediting %p #%lld", itsimpleediting,
	      iaca_item_identll (itsimpleediting));
  if (!itsimpleediting)
    {
      itsimpleediting = iaca_item_make (iacafirst_dsp);
      iaca_item_pay_load_closure_set_nth (ititemeditor,
					  IACAITEMEDITOR_SIMPLE_EDITING,
					  (IacaValue *) itsimpleediting);
      iaca_item_pay_load_put_dictionnary_str
	(iaca.ia_topdictitm, "simple_editing", (IacaValue *) itsimpleediting);
      iaca_debug ("made itsimpleediting %p #%lld", itsimpleediting,
		  iaca_item_identll (itsimpleediting));
    }
}
