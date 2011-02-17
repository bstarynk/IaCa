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

/// activate the application
enum iacaactivateapplicationval_en
{
  IACAACTIVATEAPPLICATIONVAL__LAST
};

static void
popup_final_dialog (GtkWindow * win, gpointer ptr)
{
  GtkWidget *dial = 0;
  GtkWidget *lab = 0;
  char *markup = 0;
  char *realstatedir = 0;
  guint res = 0;
  g_assert (ptr == NULL);
  g_assert (win && GTK_IS_WINDOW (win));
  dial = gtk_dialog_new_with_buttons ("Finally dump state",
				      win,
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
  GtkWindow *win = ptr;
  gint resp = 0;
  iaca_debug ("begin");
  g_assert (GTK_IS_WINDOW (win));
  dial = gtk_message_dialog_new_with_markup
    (win,
     GTK_DIALOG_DESTROY_WITH_PARENT,
     GTK_MESSAGE_QUESTION,
     GTK_BUTTONS_NONE,
     "<b>Save IaCa state</b> to directory\n <tt>%s</tt> ?\n",
     iaca.ia_statedir);
  gtk_dialog_add_buttons (GTK_DIALOG (dial), GTK_STOCK_SAVE,
			  GTK_RESPONSE_ACCEPT, GTK_STOCK_QUIT,
			  GTK_RESPONSE_NO, GTK_STOCK_CANCEL,
			  GTK_RESPONSE_REJECT, NULL);
  gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dial),
					      "<i>Save</i> to save the state to directory\n <tt>%s</tt> and continue,\n\n"
					      "<i>Quit</i> to save the state and quit,\n\n"
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
  GtkWindow *win = ptr;
  gint resp = 0;
  iaca_debug ("begin");
  g_assert (GTK_IS_WINDOW (win));
  dial = gtk_file_chooser_dialog_new ("Save Iaca state to...",
				      win,
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
  GtkWindow *win = ptr;
  gint resp = 0;
  iaca_debug ("begin");
  g_assert (GTK_IS_WINDOW (win));
  dial = gtk_message_dialog_new_with_markup
    (win,
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
iacafirst_activateapplication (GObject *gapp, IacaItem *cloitm)
{
  GtkApplication *app = GTK_APPLICATION (gapp);
  GtkWindow *win = 0;
  GtkWidget *box = 0;
  GtkWidget *menubar = 0;
  GtkWidget *filemenu = 0;
  GtkWidget *filesubmenu = 0;
  GtkWidget *savemenu = 0;
  GtkWidget *saveasmenu = 0;
  GtkWidget *quitmenu = 0;
  GtkWidget *editmenu = 0;
  GtkWidget *copymenu = 0;
  GtkWidget *cutmenu = 0;
  GtkWidget *pastemenu = 0;
  GtkWidget *editsubmenu = 0;
  GtkWidget *hbox = 0;
  GtkWidget *label = 0;
  GtkWidget *entry = 0;
  GtkWidget *notebook = 0;
  iaca_debug ("app %p", app);
  g_assert (GTK_IS_APPLICATION (app));
  win = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  g_signal_connect (win, "destroy", G_CALLBACK (popup_final_dialog),
		    (gpointer) 0);
  gtk_window_set_title (win, "iaca first");
  gtk_window_set_default_size (win, 580, 400);
  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
  gtk_container_add (GTK_CONTAINER (win), box);
  /// create the menubar
  menubar = gtk_menu_bar_new ();
  filemenu = gtk_menu_item_new_with_mnemonic ("_File");
  filesubmenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (filemenu), filesubmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), filemenu);
  savemenu = gtk_menu_item_new_with_mnemonic ("_Save");
  saveasmenu = gtk_menu_item_new_with_mnemonic ("Save _As");
  quitmenu = gtk_menu_item_new_with_mnemonic ("_Quit");
  g_signal_connect (savemenu, "activate", G_CALLBACK (save_dialog_cb), win);
  g_signal_connect (saveasmenu, "activate", G_CALLBACK (saveas_dialog_cb),
		    win);
  g_signal_connect (quitmenu, "activate", G_CALLBACK (quit_dialog_cb), win);
  gtk_menu_shell_append (GTK_MENU_SHELL (filesubmenu), savemenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (filesubmenu), saveasmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (filesubmenu), quitmenu);
  editmenu = gtk_menu_item_new_with_mnemonic ("Edit");
  editsubmenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (editmenu), editsubmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), editmenu);
  copymenu = gtk_menu_item_new_with_mnemonic ("_Copy");
  cutmenu = gtk_menu_item_new_with_mnemonic ("C_ut");
  pastemenu = gtk_menu_item_new_with_mnemonic ("_Paste");
  gtk_menu_shell_append (GTK_MENU_SHELL (editsubmenu), copymenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (editsubmenu), cutmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (editsubmenu), pastemenu);
  gtk_box_pack_start (GTK_BOX (box), menubar, FALSE, FALSE, 2);
  //// create the hbox
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 2);
  {
    char *markup = g_markup_printf_escaped ("<i>Iaca</i> <small>(%d)</small>",
					    (int) getpid ());
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), markup);
    g_free (markup);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
    entry = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 2);
  }
  //// create the notebook
  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (box), notebook, TRUE, TRUE, 2);
  ////
  gtk_window_set_application (win, app);
  gtk_widget_show_all (GTK_WIDGET (win));
}

IACA_DEFINE_CLOFUN (activateapplication,
		    IACAACTIVATEAPPLICATIONVAL__LAST,
		    gobject_do, iacafirst_activateapplication);

void
iacamod_first_init1 (void)
{
  IacaItem *itdict = 0;
  IacaItem *itgtkinit = 0;
  IacaItem *itactivappl = 0;
  IacaItem *itname = 0;
  iacafirst_dsp = iaca_dataspace ("firstspace");
  iaca_debug ("init1 of first iacafirst_dsp=%p", iacafirst_dsp);
  if (!(itdict = iaca.ia_topdictitm))
    {
      iaca.ia_topdictitm = itdict = iaca_item_make (iacafirst_dsp);
      iaca_item_pay_load_reserve_dictionnary (itdict, 53);
      iaca_item_pay_load_put_dictionnary_str (itdict, "the_dictionnary",
					      (IacaValue *) itdict);
    }
  if (!
      (itname =
       iacac_item (iaca_item_pay_load_dictionnary_get (itdict, "name"))))
    {
      itname = iaca_item_make (iacafirst_dsp);
      iaca_debug ("itname %p #%lld", itname,
		  (long long) iaca_item_ident (itname));
      iaca_item_pay_load_put_dictionnary_str (itdict, "name",
					      (IacaValue *) itname);
      iaca_item_physical_put ((IacaValue *) itdict, (IacaValue *) itname,
			      iacav_string_make ("the_dictionnary"));
      iaca_item_physical_put ((IacaValue *) itname, (IacaValue *) itname,
			      iacav_string_make ("name"));
    }
  else
    iaca_debug ("got itname %p #%lld", itname,
		(long long) iaca_item_ident (itname));
  if (!(itgtkinit = iaca.ia_gtkinititm))
    {
      iaca.ia_gtkinititm = itgtkinit = iaca_item_make (iacafirst_dsp);
      iaca_item_pay_load_make_closure (itgtkinit, &iacacfun_gtkapplinit,
				       (IacaValue **) 0);
    }
  if (!(itactivappl =
	iacac_item (iaca_item_pay_load_closure_nth
		    (itgtkinit, IACAGTKINITVAL_ACTIVEAPPL))))
    {
      itactivappl = iaca_item_make (iacafirst_dsp);
      iaca_item_pay_load_make_closure (itactivappl,
				       &iacacfun_activateapplication,
				       (IacaValue **) 0);
      iaca_item_pay_load_closure_set_nth (itgtkinit,
					  IACAGTKINITVAL_ACTIVEAPPL,
					  (IacaValue *) itactivappl);
    }
}
