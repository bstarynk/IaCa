// file module/first.c
#include "iaca.h"

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
  iaca_debug ("begin");
}

static void
quit_dialog_cb (GtkWidget *w, gpointer ptr)
{
  iaca_debug ("begin");
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
  GtkWidget *quitmenu = 0;
  iaca_debug ("app %p", app);
  g_assert (GTK_IS_APPLICATION (app));
  win = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  g_signal_connect (win, "destroy", G_CALLBACK (popup_final_dialog),
		    (gpointer) 0);
  gtk_window_set_title (win, "iaca first");
  gtk_window_set_default_size (win, 580, 400);
  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
  gtk_container_add (GTK_CONTAINER (win), box);
  menubar = gtk_menu_bar_new ();
  filemenu = gtk_menu_item_new_with_mnemonic ("_File");
  filesubmenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (filemenu), filesubmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), filemenu);
  savemenu = gtk_menu_item_new_with_mnemonic ("_Save");
  quitmenu = gtk_menu_item_new_with_mnemonic ("_Quit");
  g_signal_connect (savemenu, "activate", G_CALLBACK (save_dialog_cb), NULL);
  g_signal_connect (quitmenu, "activate", G_CALLBACK (quit_dialog_cb), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (filesubmenu), savemenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (filesubmenu), quitmenu);
  gtk_box_pack_start (GTK_BOX (box), menubar, FALSE, FALSE, 2);
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
  struct iacadataspace_st *spf = iaca_dataspace ("firstspace");
  iaca_debug ("init1 of first spf=%p", spf);
  if (!(itdict = iaca.ia_topdictitm))
    {
      iaca.ia_topdictitm = itdict = iaca_item_make (spf);
      iaca_item_pay_load_reserve_dictionnary (itdict, 53);
      iaca_item_pay_load_put_dictionnary_str (itdict, "the_dictionnary",
					      (IacaValue *) itdict);
    }
  if (!(itgtkinit = iaca.ia_gtkinititm))
    {
      iaca.ia_gtkinititm = itgtkinit = iaca_item_make (spf);
      iaca_item_pay_load_make_closure (itgtkinit, &iacacfun_gtkapplinit,
				       (IacaValue **) 0);
    }
  if (!(itactivappl =
	iacac_item (iaca_item_pay_load_closure_nth
		    (itgtkinit, IACAGTKINITVAL_ACTIVEAPPL))))
    {
      itactivappl = iaca_item_make (spf);
      iaca_item_pay_load_make_closure (itactivappl,
				       &iacacfun_activateapplication,
				       (IacaValue **) 0);
      iaca_item_pay_load_closure_set_nth (itgtkinit,
					  IACAGTKINITVAL_ACTIVEAPPL,
					  (IacaValue *) itactivappl);
    }
}
