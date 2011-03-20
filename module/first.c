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

/* the boxed gobject value for the top level window */
static IacaValue *iacafirst_valwin;
/* the boxed gobject value for the top notebook */
static IacaValue *iacafirst_valnotebook;
/* the boxed gobject value for the top entry */
static IacaValue *iacafirst_valentry;

/* the 'name' item */
static IacaItem *iacafirst_itname;

/* the transient item associating edited items to their boxed widget */
static IacaItem *iacafirst_assocedititm;


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


/* update the completion of the main entry widget for the toplevel
   dictionnary; should be called initially and at every change to the
   toplevel dictionnary */
static void
update_completion_entry_topdict (void)
{
  GtkEntry *ent = GTK_ENTRY (iaca_gobject (iacafirst_valentry));
  GtkEntryCompletion *compl = 0;
  GtkListStore *ls = 0;
  GtkTreeIter iter = { };
  int cnt = 0;
  iaca_debug ("ent %p", ent);
  g_assert (GTK_IS_ENTRY (ent));
  compl = gtk_entry_get_completion (ent);
  if (!compl)
    {
      compl = gtk_entry_completion_new ();
      gtk_entry_set_completion (ent, compl);
      g_object_unref (compl);
    }
  ls = gtk_list_store_new (1, G_TYPE_STRING);
  IACA_FOREACH_DICTIONNARY_STRING_LOCAL (iaca.ia_topdictitm, strv)
  {
    gtk_list_store_append (ls, &iter);
    gtk_list_store_set (ls, &iter, 0, iaca_string_val ((IacaValue *) strv),
			-1);
    cnt++;
  }
  gtk_entry_completion_set_model (compl, GTK_TREE_MODEL (ls));
  g_object_unref (ls);
  gtk_entry_completion_set_text_column (compl, 0);
  if (cnt > 33)
    gtk_entry_completion_set_minimum_key_length (compl, 3);
  else if (cnt > 5)
    gtk_entry_completion_set_minimum_key_length (compl, 2);
}

/// activate the application
enum iacaactivateapplicationval_en
{
  /* closure to edit an existing named item. Called with the item. Should
     return a boxed widget */
  IACAACTIVATEAPPLICATIONVAL_NAMED_EDITOR,
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



IACA_DEFINE_CLOFUN (gtkapplinit,
		    IACAGTKINITVAL__LAST, gobject_do, iacafirst_gtkinit);

static void
edit_named_cb (GtkWidget *menu, gpointer data)
{
  GtkEntry *entry = GTK_ENTRY (iaca_gobject (iacafirst_valentry));
  GtkWindow *win = GTK_WINDOW (iaca_gobject (iacafirst_valwin));
  GtkWidget *dial = 0;
  IacaItem *namededitoritm = data;
  IacaValue *widval = 0;
  GtkWidget *wid = 0;
  const gchar *txt = 0;
  const gchar *endtxt = 0;
  IacaItem *nameditm = 0;
  bool goodname = 0;
  int pagenum = -1;
  int resp = 0;
  g_assert (GTK_IS_ENTRY (entry));
  g_assert (GTK_IS_WINDOW (win));
  g_assert (namededitoritm && namededitoritm->v_kind == IACAV_ITEM);
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
      dial = gtk_message_dialog_new_with_markup
	(win,
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
      return;
    }
  nameditm = iacac_item (iaca_item_pay_load_dictionnary_get
			 (iaca.ia_topdictitm, txt));
  iaca_debug ("nameditm %p #%lld", nameditm, iaca_item_identll (nameditm));
  if (!nameditm)
    {
      dial = gtk_message_dialog_new_with_markup
	(win,
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
	  iaca_item_physical_put ((IacaValue *) nameditm,
				  (IacaValue *) iacafirst_itname,
				  iacav_string_make (txt));
	  iaca_item_pay_load_put_dictionnary_str
	    (iaca.ia_topdictitm, txt, (IacaValue *) nameditm);
	  iaca_debug ("created named '%s' %p #%lld",
		      txt, nameditm, iaca_item_identll (nameditm));
	  update_completion_entry_topdict ();
	}
      else
	nameditm = 0;
      gtk_widget_destroy (GTK_WIDGET (dial)), dial = 0;
      if (!nameditm)
	return;
    }
  widval =
    iaca_item_physical_get ((IacaValue *) iacafirst_assocedititm,
			    (IacaValue *) nameditm);
  wid = GTK_WIDGET (iaca_gobject (widval));
  iaca_debug ("got widval %p wid %p from iacafirst_assocedititm", widval,
	      wid);
  if (widval && !wid)
    {
      /* the widval was transformed into a set, so remove it */
      iaca_item_physical_remove ((IacaValue *) iacafirst_assocedititm,
				 (IacaValue *) nameditm);
      widval = NULL;
      wid = NULL;
    }
  if (!wid)
    {
      /* apply the namededitoritm to the nameditm */
      iaca_debug
	("before applying namededitoritm %p #%lld to nameditm %p #%lld",
	 namededitoritm, iaca_item_identll (namededitoritm), nameditm,
	 iaca_item_identll (nameditm));
      widval =
	iaca_item_pay_load_closure_one_value ((IacaValue *) nameditm,
					      namededitoritm);
      wid = GTK_WIDGET (iaca_gobject (widval));
      iaca_debug ("got widval %p wid %p", widval, wid);
      if (!wid)
	return;
      gtk_widget_show_all (wid);
      gtk_notebook_append_page
	(GTK_NOTEBOOK (iaca_gobject (iacafirst_valnotebook)),
	 wid, gtk_label_new (txt));
      iaca_item_physical_put ((IacaValue *) iacafirst_assocedititm,
			      (IacaValue *) nameditm, widval);
    }
  pagenum = gtk_notebook_page_num
    (GTK_NOTEBOOK (iaca_gobject (iacafirst_valnotebook)), wid);
  if (pagenum >= 0)
    gtk_notebook_set_current_page
      (GTK_NOTEBOOK (iaca_gobject (iacafirst_valnotebook)), pagenum);
}				/* end edit_named_cb */



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
  GtkWidget *namedmenu = 0;
  GtkWidget *pastemenu = 0;
  GtkWidget *editsubmenu = 0;
  GtkWidget *hbox = 0;
  GtkWidget *label = 0;
  GtkWidget *entry = 0;
  GtkWidget *notebook = 0;
  IacaItem *namededitoritm = 0;
  iaca_debug ("app %p", app);
  g_assert (GTK_IS_APPLICATION (app));
  namededitoritm =
    iacac_item
    (iaca_item_pay_load_closure_nth (cloitm,
				     IACAACTIVATEAPPLICATIONVAL_NAMED_EDITOR));
  win = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  iacafirst_valwin = iacav_gobject_box (G_OBJECT (win));
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
  namedmenu = gtk_menu_item_new_with_mnemonic ("_Named");
  copymenu = gtk_menu_item_new_with_mnemonic ("_Copy");
  cutmenu = gtk_menu_item_new_with_mnemonic ("C_ut");
  pastemenu = gtk_menu_item_new_with_mnemonic ("_Paste");
  gtk_menu_shell_append (GTK_MENU_SHELL (editsubmenu), namedmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (editsubmenu), copymenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (editsubmenu), cutmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (editsubmenu), pastemenu);
  g_signal_connect (namedmenu, "activate", G_CALLBACK (edit_named_cb),
		    namededitoritm);
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
    iacafirst_valentry = iacav_gobject_box (G_OBJECT (entry));
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 2);
    update_completion_entry_topdict ();
  }
  //// create the notebook and the association of edited items
  notebook = gtk_notebook_new ();
  iacafirst_valnotebook = iacav_gobject_box (G_OBJECT (notebook));
  iacafirst_assocedititm = iaca_item_make (iaca.ia_transientdataspace);
  gtk_box_pack_start (GTK_BOX (box), notebook, TRUE, TRUE, 2);
  ////
  gtk_window_set_application (win, app);
  gtk_widget_show_all (GTK_WIDGET (win));
}

IACA_DEFINE_CLOFUN (activateapplication,
		    IACAACTIVATEAPPLICATIONVAL__LAST,
		    gobject_do, iacafirst_activateapplication);

/// closed values for named editor
enum iacanamededitorval_en
{
  IACANAMEDEDITOR_DISPLAYITEMCONTENT,
  IACANAMEDEDITOR__LAST
};

#if 0
/*
  g_signal_connect (G_OBJECT (txview), "motion-notify-event",
		    G_CALLBACK (motion_namededitor_view), valtxbuf);
*/
static gboolean
motion_namededitor_view (GtkWidget *widg, GdkEventMotion * ev, gpointer data)
{
  gint bufx = 0, bufy = 0;
  int lin = 0, col = 0;
  GtkTextIter herit = { };
  g_assert (GTK_IS_TEXT_VIEW (widg));
  gtk_text_view_window_to_buffer_coords
    (GTK_TEXT_VIEW (widg),
     GTK_TEXT_WINDOW_WIDGET, (gint) ev->x, (gint) ev->y, &bufx, &bufy);
  gtk_text_view_get_iter_at_position (GTK_TEXT_VIEW (widg),
				      &herit, NULL, bufx, bufy);
  lin = gtk_text_iter_get_line (&herit);
  col = gtk_text_iter_get_line_offset (&herit);
  iaca_debug ("bufx %d bufy %d lin %d col %d", bufx, bufy, lin, col);
  /* return true to stop other handlers */
  return FALSE;
}
#endif

static void
txbuf_begin_user_action (GtkTextBuffer *txbuf, gpointer data)
{
  iaca_debug ("start txbuf %p", txbuf);
}

/* should return a boxed widget */
static IacaValue *
iacafirst_namededitor (IacaValue *v1, IacaItem *cloitm)
{
  GtkTextBuffer *txbuf = 0;
  GtkWidget *txview = 0;
  GtkWidget *scrwin = 0;
  IacaValue *res = 0;
  IacaValue *valtxbuf = 0;
  IacaItem *itdisplitem = 0;
  IacaItem *nitm = iacac_item (v1);
  const char *nam = 0;
  GtkTextIter endit = { };
  char cbuf[64];
  memset (cbuf, 0, sizeof (cbuf));
  iaca_debug ("start v1 %p nitm#%lld cloitm %p",
	      v1, iaca_item_identll (nitm), cloitm);
  nam = iaca_string_val
    (iaca_item_physical_get ((IacaValue *) nitm,
			     (IacaValue *) iacafirst_itname));
  if (nam
      && iaca_item_pay_load_dictionnary_get (iaca.ia_topdictitm, nam)
      != (IacaValue *) nitm)
    nam = 0;
  iaca_debug ("nam '%s'", nam);
  txbuf = gtk_text_buffer_new (NULL);
  gtk_text_buffer_create_tag (txbuf, "title",
			      "editable", FALSE,
			      "foreground", "navy",
			      "background", "ivory",
			      "scale", PANGO_SCALE_X_LARGE,
			      "family", "Verdana",
			      "justification", GTK_JUSTIFY_CENTER,
			      "weight", PANGO_WEIGHT_BOLD, NULL);
  gtk_text_buffer_create_tag (txbuf, "id",
			      "foreground", "blue",
			      "scale", PANGO_SCALE_SMALL,
			      "style", PANGO_STYLE_ITALIC, NULL);
  gtk_text_buffer_create_tag (txbuf, "decor",
			      "foreground", "firebrick", NULL);
  gtk_text_buffer_create_tag (txbuf, "literal",
			      "foreground", "darkgreen",
			      "background", "lightpink",
			      "family", "Andale Mono",
			      "style", PANGO_STYLE_ITALIC, NULL);
  gtk_text_buffer_create_tag (txbuf, "itemname",
			      "background", "orange",
			      "weight", PANGO_WEIGHT_BOLD, NULL);
  gtk_text_buffer_create_tag (txbuf, "itemserial",
			      "background", "red",
			      "style", PANGO_STYLE_ITALIC, NULL);
  g_signal_connect ((GObject *) txbuf, "begin-user-action",
		    G_CALLBACK (txbuf_begin_user_action), nitm);
  iaca_debug ("txbuf %p", txbuf);
  gtk_text_buffer_get_end_iter (txbuf, &endit);
  gtk_text_buffer_insert_with_tags_by_name
    (txbuf, &endit, nam, -1, "title", NULL);
  snprintf (cbuf, sizeof (cbuf) - 1, " #%lld", iaca_item_identll (nitm));
  gtk_text_buffer_get_end_iter (txbuf, &endit);
  gtk_text_buffer_insert_with_tags_by_name
    (txbuf, &endit, cbuf, -1, "title", "id", NULL);
  txview = gtk_text_view_new_with_buffer (txbuf);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (txview), FALSE);
  scrwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrwin),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrwin), txview);
  res = iacav_gobject_box (G_OBJECT (scrwin));
  valtxbuf = iacav_gobject_box (G_OBJECT (txbuf));
  iaca_gobject_put_data (res, valtxbuf);
  iaca_debug ("scrwin %p txview %p res %p", scrwin, txview, res);
  itdisplitem =
    iacac_item (iaca_item_pay_load_closure_nth (cloitm,
						IACANAMEDEDITOR_DISPLAYITEMCONTENT));
  if (itdisplitem)
    iaca_item_pay_load_closure_two_values (valtxbuf, (IacaValue *) nitm,
					   itdisplitem);

  return res;
}

IACA_DEFINE_CLOFUN (namededitor,
		    IACANAMEDEDITOR__LAST, one_value, iacafirst_namededitor);

////////////////////////////////////////////////////////////////
/// closed values for displayitemcontent
enum iacadisplayitemcontentval_en
{
  IACADISPLAYITEMCONTENT_ITEMREFDISPLAYER,
  IACADISPLAYITEMCONTENT_VALUEDISPLAYER,
  IACADISPLAYITEMCONTENT_TAGASSOCIATION,
  IACADISPLAYITEMCONTENT__LAST
};

static int
display_item_cmp (const void *p1, const void *p2)
{
  IacaItem *i1 = *(IacaItem **) p1;
  IacaItem *i2 = *(IacaItem **) p2;
  const char *n1 = 0;
  const char *n2 = 0;
  if (i1 == i2)
    return 0;
  n1 = iaca_string_val
    (iaca_item_physical_get ((IacaValue *) i1,
			     (IacaValue *) iacafirst_itname));
  if (n1
      && iaca_item_pay_load_dictionnary_get (iaca.ia_topdictitm, n1)
      != (IacaValue *) i1)
    n1 = NULL;
  n2 = iaca_string_val
    (iaca_item_physical_get ((IacaValue *) i2,
			     (IacaValue *) iacafirst_itname));
  if (n2
      && iaca_item_pay_load_dictionnary_get (iaca.ia_topdictitm, n2)
      != (IacaValue *) i2)
    n2 = NULL;
  if (!n1 && !n2)
    {
      if (i1->v_ident < i2->v_ident)
	return -1;
      else if (i1->v_ident > i2->v_ident)
	return 1;
    }
  if (n1 && !n2)
    return -1;
  if (!n1 && n2)
    return 1;
  return g_strcmp0 (n1, n2);
}


/* display an item, returned value is ignored, v1 is the boxed gtk
   text buffer v2 is the item to display */
static IacaValue *
iacafirst_displayitemcontent (IacaValue *v1txbuf, IacaValue *v2itd,
			      IacaItem *cloitm)
{
  GtkTextBuffer *txbuf = 0;
  GtkTextIter endit = { };
  IacaItem *ititrefdisplayer = 0;
  IacaItem *itvaluedisplayer = 0;
  IacaItem *itd = iacac_item (v2itd);
  IacaItem **attrs = 0;
  int nbattrs = 0;
  int atcnt = 0;
  ititrefdisplayer =
    iacac_item (iaca_item_pay_load_closure_nth
		(cloitm, IACADISPLAYITEMCONTENT_ITEMREFDISPLAYER));
  itvaluedisplayer =
    iacac_item (iaca_item_pay_load_closure_nth
		(cloitm, IACADISPLAYITEMCONTENT_VALUEDISPLAYER));
  g_assert (GTK_IS_TEXT_BUFFER (iaca_gobject (v1txbuf)));
  txbuf = GTK_TEXT_BUFFER (iaca_gobject (v1txbuf));
  g_assert (itd != NULL);
  nbattrs = iaca_item_number_attributes ((IacaValue *) itd);
  if (nbattrs > 0)
    attrs = iaca_alloc_data (sizeof (IacaItem *) * (nbattrs + 1));
  /* first, get the attributes and sort them.  For named attributes,
     the sorting is alphanumeric. */
  memset (attrs, 0, sizeof (IacaItem *) * nbattrs);
  atcnt = 0;
  IACA_FOREACH_ITEM_ATTRIBUTE_LOCAL ((IacaValue *) itd, vat)
  {
    IacaItem *curat = iacac_item (vat);
    if (!curat)
      continue;
    if (atcnt >= nbattrs)
      break;
    attrs[atcnt++] = curat;
  }
  qsort (attrs, atcnt, sizeof (IacaItem *), display_item_cmp);
  for (int ix = 0; ix < atcnt; ix++)
    {
      IacaItem *curat = attrs[ix];
      IacaValue *curval = 0;
      if (!curat)
	continue;
      curval = iaca_item_physical_get ((IacaValue *) itd,
				       (IacaValue *) curat);
      if (!curval)
	continue;
      gtk_text_buffer_get_end_iter (txbuf, &endit);
      gtk_text_buffer_insert (txbuf, &endit, "\n", 1);
#define DECOR_BEGIN_ATTR " \342\227\210 "	/* ◈ U+25C8 WHITE DIAMOND CONTAINING BLACK SMALL DIAMOND */
      gtk_text_buffer_insert_with_tags_by_name
	(txbuf, &endit, DECOR_BEGIN_ATTR, -1, "decor", NULL);
      iaca_item_pay_load_closure_three_values
	((IacaValue *) v1txbuf,
	 (IacaValue *) curat, (IacaValue *) NULL, ititrefdisplayer);
      gtk_text_buffer_get_end_iter (txbuf, &endit);
#define DECOR_ATTR_TO_VALUE " \342\206\246 "	/* ↦ U+21A6 RIGHTWARDS ARROW FROM BAR */
      gtk_text_buffer_insert_with_tags_by_name
	(txbuf, &endit, DECOR_ATTR_TO_VALUE, -1, "decor", NULL);
      iaca_item_pay_load_closure_three_values
	((IacaValue *) v1txbuf,
	 (IacaValue *) curval, (IacaValue *) NULL, itvaluedisplayer);
    }
  gtk_text_buffer_get_end_iter (txbuf, &endit);
  gtk_text_buffer_insert (txbuf, &endit, "\n", 1);
#define DECOR_ITEM_CONTENT " \342\201\231 "	/* U+2059 FIVE DOT PUNCTUATION ⁙ */
  gtk_text_buffer_insert_with_tags_by_name
    (txbuf, &endit, DECOR_ITEM_CONTENT, -1, "decor", NULL);
  iaca_item_pay_load_closure_three_values
    ((IacaValue *) v1txbuf,
     iaca_item_content ((IacaValue *) itd),
     (IacaValue *) NULL, itvaluedisplayer);
  gtk_text_buffer_get_end_iter (txbuf, &endit);
  gtk_text_buffer_insert (txbuf, &endit, "\n", 1);
  switch (iaca_item_pay_load_kind (itd))
    {
    case IACAPAYLOAD_DICTIONNARY:
#define DECOR_DICT_BEGIN " dict\342\246\203"	/* U+2983 LEFT WHITE CURLY BRACKET ⦃ */
      gtk_text_buffer_insert_with_tags_by_name
	(txbuf, &endit, DECOR_DICT_BEGIN, -1, "decor", NULL);
      IACA_FOREACH_DICTIONNARY_STRING_LOCAL (itd, strv)
      {
	gtk_text_buffer_get_end_iter (txbuf, &endit);
	gtk_text_buffer_insert (txbuf, &endit, "\n", 1);
#define DECOR_DICTENT_BEGIN " \342\201\202 "	/* U+2042 ASTERISM ⁂ */
	gtk_text_buffer_insert_with_tags_by_name
	  (txbuf, &endit, DECOR_DICTENT_BEGIN, -1, "decor", NULL);
	gtk_text_buffer_get_end_iter (txbuf, &endit);
	gtk_text_buffer_insert_with_tags_by_name
	  (txbuf, &endit, iaca_string_valempty ((IacaValue *) strv), -1,
	   "literal", NULL);
	gtk_text_buffer_get_end_iter (txbuf, &endit);
#define DECOR_DICTENT_GIVES " \342\244\217 "	/* U+290F RIGHTWARDS TRIPLE DASH ARROW ⤏ */
	gtk_text_buffer_insert_with_tags_by_name
	  (txbuf, &endit, DECOR_DICTENT_GIVES, -1, "decor", NULL);
	iaca_item_pay_load_closure_three_values
	  ((IacaValue *) v1txbuf,
	   (IacaValue *) iaca_item_pay_load_dictionnary_get
	   (itd,
	    iaca_string_val ((IacaValue *) strv)),
	   (IacaValue *) NULL, itvaluedisplayer);
      }
      gtk_text_buffer_get_end_iter (txbuf, &endit);
      gtk_text_buffer_insert (txbuf, &endit, "\n", 1);
#define DECOR_DICT_END " \342\246\204"	/* U+2984 RIGHT WHITE CURLY BRACKET ⦄ */
      gtk_text_buffer_get_end_iter (txbuf, &endit);
      gtk_text_buffer_insert_with_tags_by_name
	(txbuf, &endit, DECOR_DICT_END, -1, "decor", NULL);
      break;
    case IACAPAYLOAD_BUFFER:
    case IACAPAYLOAD_VECTOR:
    case IACAPAYLOAD_QUEUE:
    case IACAPAYLOAD_CLOSURE:
    case IACAPAYLOAD__NONE:
      break;
    }
  iaca_warning ("iacafirst_displayitemcontent not fully implemented");
#warning iacafirst_displayitemcontent not fully implemented
  return NULL;
}

IACA_DEFINE_CLOFUN (displayitemcontent,
		    IACADISPLAYITEMCONTENT__LAST, two_values,
		    iacafirst_displayitemcontent);


////////////////////////////////////////////////////////////////

/// closed values for itemrefdisplayer
enum iacaitemrefdisplayerval_en
{
  IACAITEMREFDISPLAYER_ASSOCITEM,
  IACAITEMREFDISPLAYER__LAST
};

static gboolean
iacafirst_item_tag_event (GtkTextTag *tag,
			  GObject *object,
			  GdkEvent *ev, GtkTextIter *iter, gpointer data)
{
  GtkTextView *txview = GTK_TEXT_VIEW (object);
  GtkTextMark *insmk = 0;
  IacaItem *itm = (IacaItem *) data;
  GtkTextBuffer *txbuf = gtk_text_view_get_buffer (txview);
  GtkTextIter txit = { };
  if (ev->type == GDK_MOTION_NOTIFY)
    {
      iaca_debug ("tag %p itm %p GDK_MOTION_NOTIFY x=%g y=%g",
		  tag, itm, ev->motion.x, ev->motion.y);
      return FALSE;
    };
  insmk = gtk_text_buffer_get_insert (txbuf);
  gtk_text_buffer_get_iter_at_mark (txbuf, &txit, insmk);
  switch (ev->type)
    {
    case GDK_BUTTON_PRESS:
      iaca_debug
	("tag %p itm %p GDK_BUTTON_PRESS x=%g y=%g but=%d insert L%d,C%d",
	 tag, itm, ev->button.x, ev->button.y, ev->button.button,
	 gtk_text_iter_get_line (&txit),
	 gtk_text_iter_get_line_offset (&txit));
      break;
    case GDK_BUTTON_RELEASE:
      iaca_debug
	("tag %p itm %p GDK_BUTTON_RELEASE x=%g y=%g but=%d insert L%d,C%d",
	 tag, itm, ev->button.x, ev->button.y, ev->button.button,
	 gtk_text_iter_get_line (&txit),
	 gtk_text_iter_get_line_offset (&txit));
      break;
    case GDK_KEY_PRESS:
      iaca_debug ("tag %p itm %p GDK_KEY_PRESS keyval=%ud insert L%d,C%d",
		  tag, itm, ev->key.keyval,
		  gtk_text_iter_get_line (&txit),
		  gtk_text_iter_get_line_offset (&txit));
      break;
    case GDK_KEY_RELEASE:
      iaca_debug ("tag %p itm %p GDK_KEY_RELEASE keyval=%ud insert L%d,C%d",
		  tag, itm, ev->key.keyval,
		  gtk_text_iter_get_line (&txit),
		  gtk_text_iter_get_line_offset (&txit));
      break;
    default:
      iaca_debug ("tag %p txview %p ev %p type %d itm %p #%lld",
		  tag, txview, ev, ev->type, itm, iaca_item_identll (itm));
      break;
    }
  /* return FALSE to propagate the event to other handlers */
  return FALSE;
}

static IacaValue *
iacafirst_itemrefdisplayer (IacaValue *v1txbuf, IacaValue *v2itm,
			    IacaValue *v3off, IacaItem *cloitm)
{
  IacaValue *res = 0;
  IacaValue *vass = 0;
  const char *nam = 0;
  long off = iaca_integer_val_def (v3off, -1L);
  GtkTextBuffer *txbuf = GTK_TEXT_BUFFER (iaca_gobject (v1txbuf));
  GtkTextTagTable *tagtbl = gtk_text_buffer_get_tag_table (txbuf);
  IacaItem *itm = iacac_item (v2itm);
  IacaItem *itassoc = iacac_item (iaca_item_pay_load_closure_nth (cloitm,
								  IACAITEMREFDISPLAYER_ASSOCITEM));
  GtkTextIter txit = { };
  GtkTextTag *tagit = 0;
  iaca_debug ("off %ld txbuf %p itm %p #%lld",
	      off, txbuf, itm, iaca_item_identll (itm));
  if (!itassoc)
    {
      itassoc = iaca_item_make (iaca.ia_transientdataspace);
      iaca_item_pay_load_closure_set_nth
	(cloitm, IACAITEMREFDISPLAYER_ASSOCITEM, (IacaValue *) itassoc);
    };
  vass = iaca_item_physical_get ((IacaValue *) itassoc, (IacaValue *) itm);
  if (vass)
    tagit = GTK_TEXT_TAG (iaca_gobject (vass));
  if (!tagit)
    {
      tagit = gtk_text_buffer_create_tag
	(txbuf, NULL, "stretch", PANGO_STRETCH_SEMI_EXPANDED, NULL);
      vass = iacav_gobject_box (G_OBJECT (tagit));
      iaca_item_physical_put ((IacaValue *) itassoc, (IacaValue *) itm, vass);
      iaca_debug ("made tagit %p for itm %p #%lld", tagit, itm,
		  iaca_item_identll (itm));
      g_signal_connect ((GObject *) tagit, "event",
			G_CALLBACK (iacafirst_item_tag_event), itm);
    }
  gtk_text_buffer_get_iter_at_offset (txbuf, &txit, off);
  nam = iaca_string_val
    (iaca_item_physical_get ((IacaValue *) itm,
			     (IacaValue *) iacafirst_itname));
  if (nam)
    gtk_text_buffer_insert_with_tags
      (txbuf, &txit, nam, -1,
       tagit, gtk_text_tag_table_lookup (tagtbl, "itemname"), NULL);
  else
    {
      char bufnum[48];
      memset (bufnum, 0, sizeof (bufnum));
      snprintf (bufnum, sizeof (bufnum) - 1, "#%lld",
		iaca_item_identll (itm));
      gtk_text_buffer_insert_with_tags (txbuf, &txit, bufnum, -1,
					tagit,
					gtk_text_tag_table_lookup (tagtbl,
								   "itemserial"),
					NULL);
    }
  return res;
}

IACA_DEFINE_CLOFUN (itemrefdisplayer,
		    IACAITEMREFDISPLAYER__LAST, three_values,
		    iacafirst_itemrefdisplayer);


////////////////////////////////////////////////////////////////

/// closed values for valuedisplayer
enum iacavaluedisplayerval_en
{
  IACAVALUEDISPLAYER_DISPLAYITEMREF,
  IACAVALUEDISPLAYER__LAST
};

static IacaValue *
iacafirst_valuedisplayer (IacaValue *v1txbuf, IacaValue *v2val,
			  IacaValue *v3off, IacaItem *cloitm)
{
  IacaValue *res = 0;
  long off = iaca_integer_val_def (v3off, -1L);
  GtkTextBuffer *txbuf = GTK_TEXT_BUFFER (iaca_gobject (v1txbuf));
  IacaItem *ititrefdisplayer
    = iacac_item (iaca_item_pay_load_closure_nth (cloitm,
						  IACAVALUEDISPLAYER_DISPLAYITEMREF));
  GtkTextIter txit = { };
  iaca_debug ("off %ld txbuf %p", off, txbuf);
  gtk_text_buffer_get_iter_at_offset (txbuf, &txit, off);
  if (!v2val)
#define DECOR_NULL " \342\254\240 "	/* U+2B20 WHITE PENTAGON ⬠ */
    gtk_text_buffer_insert_with_tags_by_name (txbuf, &txit,
					      DECOR_NULL, -1, "decor", NULL);
  else
    switch (v2val->v_kind)
      {
      case IACAV_INTEGER:
	{
#define DECOR_INTEGER " \342\201\230"	/* U+2058 FOUR DOT PUNCTUATION ⁘ */
	  char lbuf[48];
	  long l = iaca_integer_val (v2val);
	  memset (lbuf, 0, sizeof (lbuf));
	  snprintf (lbuf, sizeof (lbuf) - 1, "%ld", l);
	  gtk_text_buffer_insert_with_tags_by_name (txbuf, &txit,
						    DECOR_INTEGER, -1,
						    "decor", NULL);
	  gtk_text_buffer_insert_with_tags_by_name (txbuf, &txit, lbuf, -1,
						    "literal", NULL);
	  break;
	}
      case IACAV_STRING:
	{
#define DECOR_STRING_START " \342\200\234"	/* U+201C LEFT DOUBLE QUOTATION MARK “ */
#define DECOR_STRING_END "\342\200\235 "	/* U+201D RIGHT DOUBLE QUOTATION MARK ” */
	  gtk_text_buffer_insert_with_tags_by_name
	    (txbuf, &txit, DECOR_STRING_START, -1, "decor", NULL);
	  gtk_text_buffer_insert_with_tags_by_name
	    (txbuf, &txit, iaca_string_valempty (v2val), -1, "literal", NULL);
	  gtk_text_buffer_insert_with_tags_by_name
	    (txbuf, &txit, DECOR_STRING_END, -1, "decor", NULL);
	  break;
	}
      case IACAV_NODE:
	{
	  IacaItem *itcon = iacac_item (iaca_node_conn (v2val));
	  int arity = iaca_node_arity (v2val);
#define DECOR_NODE_PREFIX " \342\200\243"	/* U+2023 TRIANGULAR BULLET ‣ */
	  gtk_text_buffer_insert_with_tags_by_name
	    (txbuf, &txit, DECOR_NODE_PREFIX, -1, "decor", NULL);
	  iaca_item_pay_load_closure_three_values ((IacaValue *) v1txbuf,
						   (IacaValue *) itcon,
						   (IacaValue *) NULL,
						   ititrefdisplayer);
#define DECOR_NODE_BEFORE_ARGS " ("
	  gtk_text_buffer_insert_with_tags_by_name
	    (txbuf, &txit, DECOR_NODE_BEFORE_ARGS, -1, "decor", NULL);
	  for (int ix = 0; ix < arity; ix++)
	    {
#define DECOR_NODE_BETWEEN_ARGS ", "
	      IacaValue *vson = iaca_node_son (v2val, ix);
	      if (ix > 0)
		gtk_text_buffer_insert_with_tags_by_name
		  (txbuf, &txit, DECOR_NODE_BETWEEN_ARGS, -1, "decor", NULL);
	      iaca_item_pay_load_closure_three_values ((IacaValue *) v1txbuf,
						       vson,
						       (IacaValue *) NULL,
						       cloitm);
	    }
#define DECOR_NODE_AFTER_ARGS ") "
	  gtk_text_buffer_insert_with_tags_by_name
	    (txbuf, &txit, DECOR_NODE_AFTER_ARGS, -1, "decor", NULL);
	  break;
	}
      case IACAV_SET:
	{
#define DECOR_SET_PREFIX " {"
#define DECOR_SET_SUFFIX "} "
#define DECOR_SET_BETWEEN_ARGS "; "
#define DECOR_SET_EMPTY " \342\210\205 "	/* U+2205 EMPTY SET ∅ */
	  int card = iaca_set_cardinal (v2val);
	  int count = 0;
	  if (card <= 0)
	    gtk_text_buffer_insert_with_tags_by_name
	      (txbuf, &txit, DECOR_SET_EMPTY, -1, "decor", NULL);
	  else
	    {
	      gtk_text_buffer_insert_with_tags_by_name
		(txbuf, &txit, DECOR_SET_PREFIX, -1, "decor", NULL);
	      IACA_FOREACH_SET_ELEMENT_LOCAL (v2val, velem)
	      {
		if (count++ > 0)
		  gtk_text_buffer_insert_with_tags_by_name
		    (txbuf, &txit, DECOR_SET_BETWEEN_ARGS, -1, "decor", NULL);
		iaca_item_pay_load_closure_three_values ((IacaValue *)
							 v1txbuf,
							 (IacaValue *) velem,
							 (IacaValue *) NULL,
							 ititrefdisplayer);
	      }
	      gtk_text_buffer_insert_with_tags_by_name
		(txbuf, &txit, DECOR_SET_SUFFIX, -1, "decor", NULL);
	    }
	}
      case IACAV_ITEM:
	{
	  IacaItem *itv = iacac_item (v2val);
	  iaca_item_pay_load_closure_three_values ((IacaValue *) v1txbuf,
						   (IacaValue *) itv,
						   (IacaValue *) NULL,
						   ititrefdisplayer);
	  break;
	}
      default:
	iaca_error ("invalid kind %d", (int) v2val->v_kind);
      }
  return res;
}

IACA_DEFINE_CLOFUN (valuedisplayer,
		    IACAVALUEDISPLAYER__LAST, three_values,
		    iacafirst_valuedisplayer);


////////////////////////////////////////////////////////////////


void
iacamod_first_init1 (void)
{
  IacaItem *itdict = 0;
  IacaItem *itgtkinit = 0;
  IacaItem *itactivappl = 0;
  IacaItem *itname = 0;
  IacaItem *itnamededitor = 0;
  IacaItem *itdisplitem = 0;
  IacaItem *ititrefdisplayer = 0;
  IacaItem *itvaluedisplayer = 0;
  iacafirst_dsp = iaca_dataspace ("firstspace");
  iaca_debug ("init1 of first iacafirst_dsp=%p", iacafirst_dsp);
  if (!(itdict = iaca.ia_topdictitm))
    iaca_error ("missing top level dictionnary");
  if (!
      (itname =
       iacac_item (iaca_item_pay_load_dictionnary_get (itdict, "name"))))
    iaca_error ("missing 'name'");
  iacafirst_itname = itname;
  if (!(itgtkinit = iaca.ia_gtkinititm))
    iaca_error ("missing gtkinitializer");
  if (!(itactivappl =
	iacac_item (iaca_item_pay_load_closure_nth
		    (itgtkinit, IACAGTKINITVAL_ACTIVEAPPL))))
    iaca_error ("missing activeappl");
  if (!(itnamededitor
	= iacac_item (iaca_item_pay_load_closure_nth
		      (itactivappl,
		       IACAACTIVATEAPPLICATIONVAL_NAMED_EDITOR))))
    iaca_error ("missing namededitor");
  if (!(itdisplitem
	= iacac_item (iaca_item_pay_load_closure_nth
		      (itnamededitor, IACANAMEDEDITOR_DISPLAYITEMCONTENT))))
    iaca_error ("missing itdisplitem");
  if (!(ititrefdisplayer
	= iacac_item (iaca_item_pay_load_closure_nth
		      (itdisplitem,
		       IACADISPLAYITEMCONTENT_ITEMREFDISPLAYER))))
    iaca_error ("missing ititrefdisplayer");
  if (!(itvaluedisplayer
	= iacac_item (iaca_item_pay_load_closure_nth
		      (itdisplitem, IACADISPLAYITEMCONTENT_VALUEDISPLAYER))))
    iaca_error ("missing itvaluedisplayer");
  iaca_item_pay_load_closure_set_nth
    (itvaluedisplayer,
     IACAVALUEDISPLAYER_DISPLAYITEMREF, (IacaValue *) ititrefdisplayer);
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
