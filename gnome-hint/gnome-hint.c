#include <config.h>
#include <gnome.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>

static GList *hintlist = NULL;
static int hintnum = 0;

static GList *curhint = NULL;

static GnomeCanvasItem *hint_item;

static char *
default_hint(void)
{
	return  g_strdup(_("Press the foot in the lower left corner\n"
			   "to start working with GNOME"));
}

/*find the language, but only look as far as gotlang*/
static GList *
find_lang(GList *langlist, GList *gotlang, char *lang)
{
	while(langlist && langlist!=gotlang) {
		if(strcmp(langlist->data,lang)==0)
			return langlist;
		langlist = langlist->next;
	}
	return NULL;
}

/*parse all children and pick out the best language one*/
static char *
get_i18n_string(xmlDocPtr doc, xmlNodePtr child, char *name)
{
	GList *langlist;
	char *current;
	xmlNodePtr cur;
	GList *gotlang = NULL; /*the highest language we got*/
	
	langlist = gnome_i18n_get_language_list("LC_ALL");
	
	current = NULL;

	/*find C the locale string*/
	for(cur = child->childs; cur; cur = cur->next) {
		char *lang;
		if(!cur->name || strcmp(cur->name, name)!=0)
			continue;
		lang = xmlGetProp (cur, "xml:lang");
		if(!lang) {
			if(gotlang) continue;
			g_free(current);
			current = xmlNodeListGetString (doc, cur->childs, 1);
		} else {
			GList *l = find_lang(langlist,gotlang,lang);
			if(l) {
				g_free(current);
				current = xmlNodeListGetString (doc, cur->childs, 1);
				gotlang = l;
				if(l == langlist) /*we can't get any better then this*/
					break;
			}
		}
	}
	
	return current;
}

static void
read_hints_from_file(char *file)
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	doc = xmlParseFile(file);
	if(!doc) return;
	
	if(!doc->root ||
	   !doc->root->name ||
	   strcmp(doc->root->name,"GnomeHints")!=0) {
		xmlFreeDoc(doc);
		return;
	}

	for(cur = doc->root->childs; cur; cur = cur->next) {
		char *str;
		if(!cur->name || strcmp(cur->name,"Hint")!=0)
			continue;
		str = get_i18n_string(doc, cur, "Content");
		if(str) {
			hintlist = g_list_prepend(hintlist,str);
			hintnum++;
		}
	}
	
	xmlFreeDoc(doc);
}

static void
read_in_hints(void)
{
	DIR *dir;
	struct stat s;
	struct dirent *dent;

	char * name = gnome_datadir_file("gnome/hints");
	if(!name)
		return;
	if(stat(name, &s) != 0 ||
	   !S_ISDIR(s.st_mode)) {
		g_free(name);
		return;
	}
	
	dir = opendir(name);
	if(!dir) {
		g_free(name);
		return;
	}

	while((dent = readdir(dir)) != NULL) {
		char *file;
		/* Skip over dot files */
		if(dent->d_name[0] == '.')
			continue;
		file = g_concat_dir_and_file(name,dent->d_name);
		if(stat(file, &s) != 0 ||
		   !S_ISREG(s.st_mode))
			continue;
		read_hints_from_file(file);
		g_free(file);
	}
	g_free(name);
	closedir(dir);
	
	/*we read it all in reverse in fact, so now we just put it back*/
	hintlist = g_list_reverse(hintlist);
}

static char *
pick_random_hint(void)
{
	if(hintnum==0)
		return default_hint();
	
	srandom(time(NULL));
	
	/*the random is not truly random thanks to me %'ing it,
	  but you know what ... who cares*/
	curhint = g_list_nth(hintlist,random()%hintnum);
	
	return curhint->data;
}

static void
draw_on_canvas(GtkWidget *canvas, char *hint)
{
	GnomeCanvasItem *item;
	
	item = gnome_canvas_item_new(
		gnome_canvas_root(GNOME_CANVAS(canvas)),
		gnome_canvas_rect_get_type(),
		"x1",(double)0.0,
		"y1",(double)0.0,
		"x2",(double)400.0,
		"y2",(double)200.0,
		"fill_color","blue",
		NULL);

	item = gnome_canvas_item_new(
		gnome_canvas_root(GNOME_CANVAS(canvas)),
		gnome_canvas_rect_get_type(),
		"x1",(double)75.0,
		"y1",(double)50.0,
		"x2",(double)400.0,
		"y2",(double)200.0,
		"fill_color","white",
		NULL);
	
	hint_item = gnome_canvas_item_new(
		gnome_canvas_root(GNOME_CANVAS(canvas)),
		gnome_canvas_text_get_type(),
		"x",(double)237.5,
		"y",(double)125.0,
		"fill_color","black",
		"font_gdk",canvas->style->font,
		"clip_width",(double)325.0,
		"clip_height",(double)150.0,
		"clip",TRUE,
		"text",hint,
		NULL);

	item = gnome_canvas_item_new(
		gnome_canvas_root(GNOME_CANVAS(canvas)),
		gnome_canvas_text_get_type(),
		"x",(double)200.0,
		"y",(double)25.0,
		"fill_color","white",
		"font","-*-helvetica-bold-r-normal-*-*-180-*-*-p-*-*-*",
		"text","Gnome Hints",
		NULL);
}

static void
hints_clicked(GtkWidget *w, int button, gpointer data)
{
	switch(button) {
	case 0:
		if(!curhint) return;
		curhint = curhint->prev;
		if(!curhint) curhint = g_list_last(hintlist);
		gnome_canvas_item_set(hint_item,
				      "text",(char *)curhint->data,
				      NULL);
		break;
	case 1:
		if(!curhint) return;
		curhint = curhint->next;
		if(!curhint) curhint = hintlist;
		gnome_canvas_item_set(hint_item,
				      "text",(char *)curhint->data,
				      NULL);
		break;
	default:
		gtk_main_quit();
	}
}

int
main(int argc, char *argv[])
{
	GtkWidget *win;
	GtkWidget *canvas;
	char *hint;

	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);

	gnome_init("gnome-hint", VERSION, argc, argv);

	win = gnome_dialog_new(_("Gnome hint"),
			       GNOME_STOCK_BUTTON_PREV,
			       GNOME_STOCK_BUTTON_NEXT,
			       GNOME_STOCK_BUTTON_CLOSE,
			       NULL);
	gtk_signal_connect(GTK_OBJECT(win),"clicked",
			   GTK_SIGNAL_FUNC(hints_clicked),
			   NULL);
	gtk_window_set_position(GTK_WINDOW(win),GTK_WIN_POS_CENTER);

	canvas = gnome_canvas_new();
	gnome_canvas_set_scroll_region(GNOME_CANVAS(canvas),
				       0.0,0.0,400.0,200.0);
	gtk_widget_set_usize(canvas,400,200);
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(win)->vbox),canvas,TRUE,TRUE,0);
	
	if(gnome_config_get_bool("/gnome-hint/stuff/first_run=TRUE")) {
		hint = default_hint();
	} else {
		read_in_hints();
		if(!hintlist)
			hint = default_hint();
		else
			hint = pick_random_hint();
	}

	gnome_config_set_bool("/gnome-hint/stuff/first_run",FALSE);
	gnome_config_sync();

	draw_on_canvas(canvas, hint);
	
	gtk_widget_show_all(win);

	gtk_main();

	return 0;
}
