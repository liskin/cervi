/*
 * GTK Cervi
 * vim:set sw=4 sta:
 *
 * Copyright (C) 2003  Tomas Janousek <tomi@nomi.cz>
 * See file COPYRIGHT and COPYING
 */

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cctype>
#include <sys/time.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "field.h"
#include "game.h"
#include "music.h"
using namespace std;

// delete and set to NULL
#define DELETE(a) { \
    delete a; \
    a = 0; \
}

// global vars:
GtkWidget *drawing_area; // field
GdkPixmap *pixmap = 0; // backing pixmap for drawing_area
unsigned int tmout = 0; // timeout id
Game *game = NULL; // pointer to game, if NULL, currently not playing
GtkWidget *score[8]; // labels for score
int  score_n[8] = { 0,0,0,0,0,0,0,0 }; // score counters
char score_s[8][14] = { "-","-","-","-","-","-","-","-" }; // temps for score
int n_cervi = 2; // number of cerv's
GameParam gpar = { placing: 0, width: defwidth, height: defheight };
    // game parameters for next round
int newwidth = defwidth, newheight = defheight;
    // will change field size after round
GdkFont *bigfont, *medfont; // fonts
int medfontheight = 0; // medfont - height of text "GNU Iy" + 3
int lastwin = -1, lastwins = 0; // hatrick counter

// field sizes
struct {
    int width, height;
} sizes[] = {
    {defwidth,defheight},
    {780,530},
    {1000,680},
    {1250,930},
    {1580,1100}
};

// forward declarations
int draw(void *area);
void newround();
void newgame(gpointer, gpointer n);
void chplacing(gpointer, gpointer n);
void updatefsize();
void chfsize(gpointer, gpointer n);
void about();
void quit();

// Menu
#define A (GtkItemFactoryCallback)
GtkItemFactoryEntry menu_items[] = {
 { "/_Game",			NULL,		NULL,	0, "<Branch>" },
 { "/Game/_New round",		"Return", newround,	0, NULL },
 { "/Game/sep1",		NULL,	    	NULL,	0, "<Separator>" },
 { "/Game/New _2 player game",	"F2",	   A newgame,	2, NULL },
 { "/Game/New _3 player game",	"F3",	   A newgame,	3, NULL },
 { "/Game/New _4 player game",	"F4",	   A newgame,	4, NULL },
 { "/Game/New _5 player game",	"F5",	   A newgame,	5, NULL },
 { "/Game/New _6 player game",	"F6",	   A newgame,	6, NULL },
 { "/Game/New _7 player game",	"F7",	   A newgame,	7, NULL },
 { "/Game/New _8 player game",	"F8",	   A newgame,	8, NULL },
 { "/Game/sep2",		NULL,	    	NULL,	0, "<Separator>" },
 { "/Game/_Quit",		"<control>Q",	quit,	0, NULL },
 { "/_Options",			NULL,		NULL,	0, "<Branch>"},
 { "/Options/Look at center",	NULL,	 A chplacing,	0, "<RadioItem>"},
 { "/Options/Look from center",	NULL,	 A chplacing,	1,
     "/Options/Look at center"},
 { "/Options/sep1",		NULL,	    	NULL,	0, "<Separator>" },
 { "/Options/Field 600x410",	NULL,	 A chfsize,	0, "<RadioItem>"},
 { "/Options/Field 780x530",	NULL,	 A chfsize,	1,
     "/Options/Field 600x410"},
 { "/Options/Field 1000x680",	NULL,	 A chfsize,	2,
     "/Options/Field 600x410"},
 { "/Options/Field 1250x930",	NULL,	 A chfsize,	3,
     "/Options/Field 600x410"},
 { "/Options/Field 1580x1100",	NULL,	 A chfsize,	4,
     "/Options/Field 600x410"},
 { "/_Help",			NULL,		NULL,	0, "<LastBranch>" },
 { "/_Help/_About...",		NULL,	       about,	0, NULL }
};
#undef A

void get_main_menu( GtkWidget  *window, GtkWidget **menubar )
{
 GtkItemFactory *item_factory;
 GtkAccelGroup *accel_group;
 gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);
 accel_group = gtk_accel_group_new ();
 item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);
 gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
 gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
 if (menubar)
  *menubar = gtk_item_factory_get_widget (item_factory, "<main>");
}

// highres time functions
unsigned long long int ticks()
{
    struct timeval t;
    gettimeofday(&t,0);
    return (t.tv_sec*1000)+(t.tv_usec/1000);
}

unsigned long long int lastticks = 0;

unsigned long long int nowticks()
{
    unsigned long long int oldticks = lastticks;

    if (lastticks == 0) {
	lastticks = ticks();
	return 0;
    }

    lastticks = ticks();
    return lastticks - oldticks;
}

unsigned long long int tempticks()
{
    if (lastticks == 0) {
	lastticks = ticks();
	return 0;
    }

    return ticks() - lastticks;
}

void resetticks() {
    lastticks = 0;
}

// update status bar
void update_status(int n, int win)
{
    for (int i=0; i<n; i++) {
	sprintf(score_s[i],"%i",score_n[i]);
	if (win == i) // mark winner
	    strcat(score_s[i],"+");
	gtk_label_set_text(GTK_LABEL(score[i]),score_s[i]);
    }
    for (int i=n_cervi; i<8; i++) {
	strcpy(score_s[i],"-");
	gtk_label_set_text(GTK_LABEL(score[i]),score_s[i]);
    }
}

// write something with big bold font
void bigbold(const char* str, unsigned int color)
{
    GdkGC *gc = gdk_gc_new(drawing_area->window);
    if (!gc) throw 0;
    GdkRectangle update_rect = {gpar.width,gpar.height,
	gdk_string_width(bigfont,str),
	gdk_string_height(bigfont,str)
    };
    update_rect.x = (update_rect.x - update_rect.width)/2;
    update_rect.y = (update_rect.y + update_rect.height)/2;
    gdk_rgb_gc_set_foreground(gc,((((color & 0xff0000) >> 1) & 0xff0000)) |
	    ((((color & 0xff00) >> 1) & 0xff00)) |
	    ((((color & 0xff) >> 1) & 0xff)));
    gdk_draw_string(pixmap,bigfont,gc,update_rect.x-2,
	    update_rect.y-2,str);
    gdk_draw_string(pixmap,bigfont,gc,update_rect.x+2,
	    update_rect.y-2,str);
    gdk_draw_string(pixmap,bigfont,gc,update_rect.x-2,
	    update_rect.y+2,str);
    gdk_draw_string(pixmap,bigfont,gc,update_rect.x+2,
	    update_rect.y+2,str);
    gdk_draw_string(pixmap,bigfont,gc,update_rect.x-2,
	    update_rect.y,str);
    gdk_draw_string(pixmap,bigfont,gc,update_rect.x+2,
	    update_rect.y,str);
    gdk_draw_string(pixmap,bigfont,gc,update_rect.x,
	    update_rect.y-2,str);
    gdk_draw_string(pixmap,bigfont,gc,update_rect.x,
	    update_rect.y+2,str);
    gdk_rgb_gc_set_foreground(gc,color);
    gdk_draw_string(pixmap,bigfont,gc,update_rect.x,update_rect.y,str);
    update_rect.y -= update_rect.height + 2;
    update_rect.x -= 2;
    update_rect.width += 4;
    update_rect.height += 4;
    gtk_widget_draw(drawing_area, &update_rect);
    gdk_gc_unref(gc);
}

// start/stop round
void startstop(gpointer a)
{
    if (tmout) {
	gtk_timeout_remove(tmout);
	tmout = 0;
	resetticks();

	if (game->over) {
	    int pos[game->n_cervi];
	    int win = -1;
	    game->result(pos);
	    for (int i=0; i<game->n_cervi; i++) {
		if (pos[i] == 1) {
		    score_n[i]++;
		    if (win != -1) // there can be more than 1 winner so we
			win = -2;  // won't show them
		    else
			win = i;
		}
	    }
	    update_status(game->n_cervi,win);

	    // check for hatrick
	    if (win == lastwin) {
		lastwins++;
		if (lastwins == 3) { // hatrick
		    bigbold("HATRICK",game->cervi[win]->_color);

		    score_n[win] += 3;
		    update_status(game->n_cervi,win);
		} else if (lastwins == 10) { // extra hatrick
		    bigbold("EXTRA HATRICK",game->cervi[win]->_color);

		    score_n[win] += 10;
		    update_status(game->n_cervi,win);
		}
	    } else {
		lastwin = win;
		lastwins = 1;
	    }	
	}

	DELETE(game);
	updatefsize();
    } else {
	for (int i=n_cervi; i<8; score_n[i++] = 0);
	game = new Game(n_cervi,gpar);
	update_status(game->n_cervi,-1);
	gdk_draw_rectangle(pixmap,((GtkWidget*)a)->style->black_gc,1,0,0,
		((GtkWidget*)a)->allocation.width,
		((GtkWidget*)a)->allocation.height);
	gdk_draw_rectangle(pixmap,((GtkWidget*)a)->style->white_gc,0,0,0,
		((GtkWidget*)a)->allocation.width-1,
		((GtkWidget*)a)->allocation.height-1);
	gtk_widget_draw(((GtkWidget*)a), NULL);
	tmout = gtk_timeout_add(10,draw,a);
    }
}

// start new round
void newround()
{
    startstop(drawing_area);
    if (!game)
	startstop(drawing_area);
}

// start new game with given number of players
void newgame(gpointer, gpointer n)
{
    if (game)
	startstop(drawing_area);
    n_cervi = (int)n;
    for (int i=0; i<8; score_n[i++] = 0);
    update_status(n_cervi,-1);

    gdk_draw_rectangle(pixmap,drawing_area->style->black_gc,1,0,0,
	    drawing_area->allocation.width,
	    drawing_area->allocation.height);
    gdk_draw_rectangle(pixmap,drawing_area->style->white_gc,0,0,0,
	    drawing_area->allocation.width-1,
	    drawing_area->allocation.height-1);
    gtk_widget_draw(drawing_area, NULL);
}

// change placing
void chplacing(gpointer, gpointer n)
{
    gpar.placing = (int)n;
}

// do everything for changing field size
void updatefsize()
{
    if (game)
	return;
    if (newwidth == gpar.width && newheight == gpar.height)
	return;
    gpar.width = newwidth;
    gpar.height = newheight;
    gtk_widget_set_usize(GTK_WIDGET(drawing_area),newwidth,newheight);
}

// change field size
void chfsize(gpointer, gpointer n)
{
    newwidth = sizes[(int)n].width;
    newheight = sizes[(int)n].height;
    updatefsize();
}

// create backing pixmap (and show about)
gint configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
    if (pixmap)
	gdk_pixmap_unref(pixmap);

    pixmap = gdk_pixmap_new(widget->window,widget->allocation.width,
	    widget->allocation.height,-1);
    about();

    return TRUE;
}

// redraw exposed area
gint expose_event(GtkWidget *widget, GdkEventExpose *event)
{
    gdk_draw_pixmap(widget->window,
	    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
	    pixmap,
	    event->area.x, event->area.y,
	    event->area.x, event->area.y,
	    event->area.width, event->area.height);

    return FALSE;
}

// set/reset key in keymap
gint key_event(GtkWidget *widget, GdkEventKey *event, gpointer p)
{
    if (game) {
	int k = event->keyval;
	if (k < 0x100 && isalpha(k))
	    k = toupper(k);
	if (k != 0)
	    game->keymap.set(k,(bool)p);
	gtk_signal_emit_stop_by_name(GTK_OBJECT(widget),
		((bool)p)?"key_press_event":"key_release_event");
    }
    return 0;
}

// do update/draw
int draw(void *area)
{
    if (!game)
	return 1;

    // I don't believe in GTK timeout so everything is timed on highres time
    unsigned long long int a = nowticks();
    game->update((GtkWidget*)area,pixmap,a);
    if (game->over) {
	startstop(area);
    }

    return 1;
}

// cleanup
void quit()
{
    DELETE(game);
    gdk_font_unref(bigfont);
    gdk_font_unref(medfont);
    gtk_exit(0);
}

// show about...
void about()
{
    gdk_draw_rectangle(pixmap,drawing_area->style->black_gc,1,0,0,
	    drawing_area->allocation.width,
	    drawing_area->allocation.height);
    gdk_draw_rectangle(pixmap,drawing_area->style->white_gc,0,0,0,
	    drawing_area->allocation.width-1,
	    drawing_area->allocation.height-1);

    int n = 0;
#define B n++
#define A(a) gdk_draw_string(pixmap,medfont,drawing_area->style->white_gc,50,\
	50+n*medfontheight,a); B

    A("GTK Cervi v" VERSION);
    A(" the Cervi clone");
    A("Copyright (C) 2003  Tomas Janousek");
    B;

    if (gpar.width < 600 || gpar.height < 410) {
	A("Need bigger field size");
	B;
	goto final_about;
    }

    A("Author: Tomas Janousek <tomi@nomi.cz>");
    B;

    A("Send comments and/or bug reports to cervi@tomi.nomi.cz");
    B;

    A("This version has these known bugs:");
    A(" - Controls are defined only for 4 players (game.cc line 38)");
    A(" - If someone sets window size to another, game won't do anything and");
    A("   you will see only the part of game area which fits into that size");
    A("   (This will be no longer considered as bug)");
    A(" - Key repeating causes cerv's to turn badly");
    B;

    A("What will be probably done in next version (not in bugfix release):");
    A(" - Controls for 8 people (not for 4), maybe configurable controls");
    B;

    A("What will be probably done... (some day)");
    A(" - Translation, mainly Czech and Slovak");
    A(" - Some additions to Cervi gameplay (optional), not in original Cervi");
    A(" - Network play (peer2peer and maybe someday massive multiplayer)");
    B;

    A("For other information, read README");
    B;

#undef A
#undef B

final_about:
    gtk_widget_draw(drawing_area, NULL);
}

// sigusr1 handler
void sigusr1(int)
{
    return;
}

// music thread
pthread_t mt;
void* music_t(void*)
{
    signal(SIGUSR1,sigusr1);
    m.thread();
    return 0;
}

// signal music thread to stop waiting
void signal_mt()
{
    pthread_kill(mt,SIGUSR1);
}

// all loved C main function
int main(int argc, char *argv[])
{
    GtkWidget *window,*vbox,*status,*label,*menu;
    GdkImage *im;

    gtk_init(&argc, &argv);
    gdk_rgb_init();
    gtk_set_locale();
    srand(time(0) ^ getpid());
    // we can disable key repeat, but we should reenable it on exit and on
    // lost focus, but this is impossible if SIGSEGV can happen and doing
    // gdk_key_repeat_restore() in SIGSEGV handler looks bad, so we'll do it
    // when everything is stable
    //gdk_key_repeat_disable();

    // music thread init
    pthread_create(&mt,0,music_t,0);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GTK Cervi");
    gtk_window_set_policy(GTK_WINDOW(window),0,0,1);

    // set up fonts
    bigfont = gdk_font_load("-*-helvetica-bold-r-normal--34-*");
    if (!bigfont) bigfont = gdk_font_ref(window->style->font);

    medfont = gdk_font_load("-misc-fixed-medium-r-normal--14-*");
    if (!medfont) medfont = gdk_font_ref(window->style->font);
    medfontheight = gdk_string_height(medfont,"GNU Iy") + 3;

    vbox = gtk_vbox_new(0, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);

    gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(quit),
	    NULL);
    gtk_signal_connect(GTK_OBJECT(window), "key_press_event",
	    GTK_SIGNAL_FUNC(key_event), (void*)1);
    gtk_signal_connect(GTK_OBJECT(window), "key_release_event",
	    GTK_SIGNAL_FUNC(key_event), (void*)0);
    gtk_widget_add_events(window, GDK_KEY_RELEASE_MASK);

    // create menu
    get_main_menu(window,&menu);
    gtk_box_pack_start(GTK_BOX(vbox),menu,0,1,0);
    gtk_widget_show(menu);

    // create drawing (game) area
    drawing_area = gtk_drawing_area_new();
    GTK_OBJECT_UNSET_FLAGS(drawing_area,GTK_CAN_FOCUS);
    gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), defwidth, defheight);
    gtk_box_pack_start(GTK_BOX(vbox), drawing_area, 0, 0, 0);

    gtk_widget_show(drawing_area);

    gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event",
	    (GtkSignalFunc)expose_event, NULL);
    gtk_signal_connect (GTK_OBJECT(drawing_area),"configure_event",
	    (GtkSignalFunc)configure_event, NULL);

    // create status bar
    status = gtk_hbox_new(0,0);
    gtk_box_pack_start(GTK_BOX(vbox), status, 0, 0, 0);
    gtk_widget_show(status);

    label = gtk_label_new("Score: ");
    gtk_box_pack_start(GTK_BOX(status), label, 0, 0, 5);
    gtk_widget_show(label);

    label = gtk_label_new("");
    gtk_box_pack_end(GTK_BOX(status), label, 0, 0, 5);
    gtk_widget_show(label);

    for (int i=7; i>=0; i--) {
	score[i] = gtk_label_new(score_s[i]);
	gtk_box_pack_end(GTK_BOX(status), score[i], 0, 0, 5);
	gtk_widget_show(score[i]);

	im = gdk_image_new(GDK_IMAGE_NORMAL,gdk_visual_get_system(),8,8);
	for (int x=0; x<8; x++)
	    for (int y=0; y<8; y++)
		gdk_image_put_pixel(im,x,y,gdk_rgb_xpixel_from_rgb(colors[i]));
	label = gtk_image_new(im,0);
	gtk_box_pack_end(GTK_BOX(status), label, 0, 0, 0);
	gtk_widget_show(label);

	label = gtk_vseparator_new();
	gtk_box_pack_end(GTK_BOX(status), label, 0, 0, 5);
	gtk_widget_show(label);
    }

    update_status(n_cervi,-1);

    gtk_widget_show(window);

    gtk_main();

    return 0;
}
