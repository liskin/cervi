/*
 * GTK Cervi
 * vim:set sw=4 sta:
 *
 * Copyright (C) 2003  Tomas Janousek <tomi@nomi.cz>
 * See file COPYRIGHT and COPYING
 */

#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

#pragma interface

#include "field.h"
#include "keymap.h"
#include <gtk/gtk.h>

namespace std {
    class Cerv;

    struct GameParam {
	int placing;
	int width, height;
    };

    class Game {
	public:
	    Field field;
	    Cerv *cervi[8];
	    int _ncollided;
	    int n_cervi;
	    KeyMap keymap;
	    bool over;

	    Game(int ncervi, const GameParam& par);
	    ~Game();
	    void update(GtkWidget* area, GdkPixmap* pixmap,
		    unsigned long long int ticks);
	    void result(int *pos);
	    double speed();
    };

    // class Worm (we call it Cerv because of the name of the game)
    // (Cerv is Czech word for English worm; cervi is plural of cerv)
    class Cerv {
	public:
	    Game* _game;
	    double _x, _y, _angle, _speed, _rotspeed, _cspeed;
	    unsigned int _color;
	    int _n;
	    bool _collision;
	    int _place;
	    
	    Cerv(Game* game, double x, double y, double angle, double speed,
		    double rotspeed, unsigned int color, int n) : _game(game),
		    _x(x), _y(y), _angle(angle), _speed(speed),
		    _rotspeed(rotspeed), _cspeed(1), _color(color), _n(n),
		    _collision(0), _place(-1) { }
	    void update(unsigned long long ticks);
	    int collision();
    };

    extern unsigned int colors[];
}

#endif /* GAME_H_INCLUDED */
