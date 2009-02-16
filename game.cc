/*
 * GTK Cervi
 * vim:set sw=4 sta:
 *
 * Copyright (C) 2003  Tomas Janousek <tomi@nomi.cz>
 * See file COPYRIGHT and COPYING
 */

#pragma implementation

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <gdk/gdkkeysyms.h>
#include "game.h"
#include "music.h"

/*
 * TODO:
 *  Controls :)
 *  (Optional) After reaching wall, do not collide but appear on the other side
 *             or bounce.
 */

// from main.cc for signalling music thread
extern void signal_mt();

namespace std {
    // colors of cerv's
    unsigned int colors[] = {
	0xff0000,
	0x00ff00,
	0x00ffff,
	0xff00ff,
	0xff8000,
	0x0000ff,
	0xffff00,
	0xffffff
    };

    // controls of cerv's
    static struct {
	unsigned int left[3], right[3];
    } controls[8] = {
	{{'N',0,0}, {'M',0,0}},
	{{'1','!',0}, {'2','@',0}},
	{{'[','{',0}, {']','}',0}},
	{{'S',0,0}, {'D',0,0}},
	{{GDK_KP_1,0,0}, {GDK_KP_2,0,0}},
	{{GDK_KP_Add,0,0}, {GDK_KP_Subtract,0,0}},
	{{0,0,0}, {0,0,0}},
	{{0,0,0}, {0,0,0}}
    };

    // create ncervi cerv's, place them
    Game::Game(int ncervi, const GameParam& par) : field(par.width,par.height),
	    _ncollided(0), over(false)
    {
	n_cervi = ncervi;
	for (int i=0; i<n_cervi; i++) {
	    if (par.placing == 0) {
		double x,y,angle,r,speed;
		bool bad = 1;
		while (bad) {
		    // place cerv's randomly with some distances between them
		    // and looking at center
		    angle = (rand() % 1000) * 2 * M_PI / 1000;
		    r = (rand() % 500) + 1500;
		    x = (field.width/2) - (field.width*cos(angle)*r/6000);
		    y = (field.height/2) - (field.height*sin(angle)*r/6000);

		    // check for distances between other cerv's
		    bad = 0;
		    for (int j=i-1; j>=0; j--)
			if (hypot(cervi[j]->_x - x, cervi[j]->_y - y) < 50 ||
				fabs(cervi[j]->_angle - angle) < 0.4) {
			    bad = 1;
			    break;
			}
		}
		speed = 70 - 5 * (n_cervi - 2);
		if (par.speed == 0)
		    speed += rand() % 50;
		else
		    speed += 25;
		cervi[i] = new Cerv(this,x,y,angle,speed,10,colors[i],i);
	    } else if (par.placing == 1) {
		double speed = 70 - 5 * (n_cervi - 2);
		if (par.speed == 0)
		    speed += rand() % 50;
		else
		    speed += 25;

		cervi[i] = new Cerv(this,field.width / 2,field.height / 2,
			-M_PI_2 + i*2*M_PI/n_cervi +//position cerv's regularly
			2*M_PI*((rand()%20)-10)/n_cervi/100, // with variation
			speed,10,colors[i],i);
		cervi[i]->_cspeed = cervi[i]->_speed;
		cervi[i]->update(75*(n_cervi+2)/2); // keep similar distance
		// between cerv's for any number of cerv's
		cervi[i]->_cspeed = 1;
#ifdef DEBUG
	    } else {
		cerr << "Bad placing" << endl;
		throw 0;
#endif /* DEBUG */
	    }
	}

	m.reset(); // reset music to initial state
	m.speed = speed();
	m.play = true;
	signal_mt(); // signal it
    }

    // delete all cervi
    Game::~Game()
    {
	for (int i=0; i<n_cervi; i++) {
	    delete cervi[i];
	}
	m.play = false;
	signal_mt();
    }

    // darken color
    inline int darkencolor(int color, unsigned char light)
    {
	return ((((color & 0xff0000)*light/255) & 0xff0000)) |
	       ((((color & 0xff00)*light/255) & 0xff00)) |
	       ((((color & 0xff)*light/255) & 0xff));
    }

    // count a to the power of 2
    inline double to2(double a)
    {
	return a*a;
    }

    // count distance and lightness from it
    inline unsigned char dist2light(int x, int y, const Cerv *cerv)
    {
	double distance = 
	    4 - to2(hypot(cerv->_x - x, cerv->_y - y));
	if (distance < 0)
	    return 0;
	return (unsigned char)(distance/ /*3.2*/ 4*255);
    }

    // update & draw
    void Game::update(GtkWidget* area, GdkPixmap* pixmap,
	    unsigned long long int ticks)
    {
	if (over) return;
	GdkGC *gc = gdk_gc_new(((GtkWidget*)area)->window);
	if (!gc) throw 0;
	
	if (ticks > 50) // will slow down game, but this can be caused only by
	    ticks = 50; // slow CPU or high load, so in result, user will have
			// control over his Cerv even in these situations
			// (... 20 fps is minimum)

	// update, check collision, draw ...
	for (int i=0; i<n_cervi; i++) {
	    if (cervi[i]->_collision) continue;
	    
	    int _x = (int)nearbyint(cervi[i]->_x),
		_y = (int)nearbyint(cervi[i]->_y);
	    for (unsigned long long int j=0; j<ticks; j++) {
		cervi[i]->update(1);
		int x = (int)nearbyint(cervi[i]->_x),
		    y = (int)nearbyint(cervi[i]->_y);
		if (!(x == _x && y == _y)) {
		    if (!cervi[i]->_collision) cervi[i]->collision();
		    
		    // set bits in bitfield
		    field._b[field.pos(x,y)] = 1;
		    field._b[field.pos(x-1,y)] = 1;
		    field._b[field.pos(x+1,y)] = 1;
		    field._b[field.pos(x,y-1)] = 1;
		    field._b[field.pos(x,y+1)] = 1;
		    
		    // draw it (with antialias)
		    for (int iy = -2; iy < 3; iy++)
			for (int ix = -2; ix < 3; ix++) {
			    unsigned char light =
				dist2light(x+ix,y+iy,cervi[i]);
			    if (field._l[field.pos(x+ix,y+iy)] < light) {
				field._l[field.pos(x+ix,y+iy)] = light;
				gdk_rgb_gc_set_foreground(gc,
					darkencolor(cervi[i]->_color,light));
				gdk_draw_point(pixmap,gc,x+ix,y+iy);
			    }
			}
 
		    _x = x;
		    _y = y;
		    GdkRectangle update_rect = {x-2,y-2,5,5};
		    gtk_widget_draw(GTK_WIDGET(area), &update_rect);
		}
		if (cervi[i]->_collision) break;
	    }
	}

	// count collided cerv's
	int ncollision = 0;
	for (int i=0; i<n_cervi; i++) {
	    if (cervi[i]->_collision)
		ncollision++;
	}

	// increase speed if someone collided
	if (ncollision - _ncollided) {
	    for (int i=0; i<n_cervi; i++)
		if (!cervi[i]->_collision) {
		    cervi[i]->_speed += (ncollision - _ncollided) * 20;
		    cervi[i]->_rotspeed -= (ncollision - _ncollided);
		}
	    m.speed = speed();
	}

	// set place numbers
	int maxplace = -1;
	for (int i=0; i<n_cervi; i++) {
	    if (cervi[i]->_collision && cervi[i]->_place > maxplace) {
		maxplace = cervi[i]->_place;
	    }
	}
	if (maxplace == -1) maxplace++;
	maxplace++;
	for (int i=0; i<n_cervi; i++) {
	    if (cervi[i]->_collision && cervi[i]->_place == -1) {
		cervi[i]->_place = maxplace;
	    }
	}

	// is round over ??
	if (ncollision == n_cervi)
	    over = true;

	// if all except one are collided, collide him to end round
	if (ncollision == n_cervi-1 && n_cervi != 1) {
	    for (int i=0; i<n_cervi; i++) {
		if (!cervi[i]->_collision) {
		    cervi[i]->_collision = true;
		    m.play = false;
		    signal_mt();
		}
	    }
	}

	_ncollided = ncollision;
	gdk_gc_unref(gc);
    }

    // save place numbers to int pos[n_cervi]
    void Game::result(int *pos)
    {
	int maxplace = -1;
	for (int i=0; i<n_cervi; i++) {
	    if (cervi[i]->_collision && cervi[i]->_place > maxplace) {
		maxplace = cervi[i]->_place;
	    }
	}
	maxplace++;
	for (int i=0; i<n_cervi; i++) {
	    pos[i] = maxplace - cervi[i]->_place;
	}
    }

    // get game speed (average speed of non-collided cerv's)
    double Game::speed()
    {
	double s = 0;
	int n = 0;
	for (int i=0; i<n_cervi; i++) {
	    if (cervi[i]->_collision) continue;
	    s += cervi[i]->_cspeed;
	    n ++;
	}
	if (n) {
	    return s/n;
	}
	return 100;
    }

    // move and/or rotate cerv and check if collided to wall
    void Cerv::update(unsigned long long ticks)
    {
	for (int i=0; i<3; i++)
	    if (_game->keymap[controls[_n].left[i]]) {
		_angle -= ticks * _rotspeed * _cspeed / 150 / 1000;
		break;
	    }
	for (int i=0; i<3; i++)
	    if (_game->keymap[controls[_n].right[i]]) {
		_angle += ticks * _rotspeed * _cspeed / 150 / 1000;
		break;
	    }

	while (_angle > 2*M_PI)
	    _angle -= 2*M_PI;
	while (_angle < 0)
	    _angle = 2*M_PI + _angle;
	
	_x += ticks * _cspeed * cos(_angle) / 1000;
	_y += ticks * _cspeed * sin(_angle) / 1000;

	if (_cspeed < _speed) {
	    _cspeed += _speed * ticks / 1000;
	    if (_cspeed > _speed)
		_cspeed = _speed;
	    m.speed = _game->speed();
	}

	if (_x < 1) {
	    _x = 1;
	    _collision = true;
	}
	if (_y < 1) {
	    _y = 1;
	    _collision = true;
	}
	if (_x > (_game->field.width-2)) {
	    _x = _game->field.width-2;
	    _collision = true;
	}
	if (_y > (_game->field.height-2)) {
	    _y = _game->field.height-2;
	    _collision = true;
	}
    }

    // check if collided to himself or another cerv
    int Cerv::collision()
    {
	if (!_collision) {
	    if (_angle >= 0 && _angle < M_PI_2) {
		if    (_game->field._b[_game->field.pos((int)nearbyint(_x)+2,
			    (int)nearbyint(_y))]
		    || _game->field._b[_game->field.pos((int)nearbyint(_x)+1,
			    (int)nearbyint(_y)+1)]
		    || _game->field._b[_game->field.pos((int)nearbyint(_x),
			    (int)nearbyint(_y)+2)]) {
		    _collision = true;
		    //  |
		    // -+-
		    //  |/
		}
	    } else if (_angle >= M_PI_2 && _angle < M_PI) {
		if    (_game->field._b[_game->field.pos((int)nearbyint(_x)-2,
			    (int)nearbyint(_y))]
		    || _game->field._b[_game->field.pos((int)nearbyint(_x)-1,
			    (int)nearbyint(_y)+1)]
		    || _game->field._b[_game->field.pos((int)nearbyint(_x),
			    (int)nearbyint(_y)+2)]) {
		    _collision = true;
		    //  |
		    // -+-
		    // \|
		}
	    } else if (_angle >= M_PI && _angle < M_PI+M_PI_2) {
		if    (_game->field._b[_game->field.pos((int)nearbyint(_x)-2,
			    (int)nearbyint(_y))]
		    || _game->field._b[_game->field.pos((int)nearbyint(_x)-1,
			    (int)nearbyint(_y)-1)]
		    || _game->field._b[_game->field.pos((int)nearbyint(_x),
			    (int)nearbyint(_y)-2)]) {
		    _collision = true;
		    // /|
		    // -+-
		    //  |
		}
	    } else if (_angle >= M_PI+M_PI_2 && _angle < 2*M_PI) {
		if    (_game->field._b[_game->field.pos((int)nearbyint(_x)+2,
			    (int)nearbyint(_y))]
		    || _game->field._b[_game->field.pos((int)nearbyint(_x)+1,
			    (int)nearbyint(_y)-1)]
		    || _game->field._b[_game->field.pos((int)nearbyint(_x),
			    (int)nearbyint(_y)-2)]) {
		    _collision = true;
		    /*  |\ */
		    // -+-
		    //  |
		}
#ifdef DEBUG
	    } else {
		// this cannot happen if EVERYTHING is right
		cerr << "This cannot happen!!! " << __FILE__ << " - " <<
		    __LINE__ << endl;
		throw 0;
#endif /* DEBUG */
	    }
	}
	return _collision;
    }
}
