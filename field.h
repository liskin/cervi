/*
 * GTK Cervi
 * vim:set sw=4 sta:
 *
 * Copyright (C) 2003  Tomas Janousek <tomi@nomi.cz>
 * See file COPYRIGHT and COPYING
 */

#ifndef FIELD_H_INCLUDED
#define FIELD_H_INCLUDED

#pragma interface

#include <vector>

namespace std {
    class Field {
	public:
	    vector<bool> _b; // bitfield
	    vector<unsigned char> _l; // lightness of pixels (for antialias)
	    int width, height;

	    Field(int _width, int _height) : _b(_width*_height + 1,false),
		    _l(_width*_height+1,0), width(_width), height(_height) {
		_b[_width*_height] = 1;
	    }

	    // short for counting bit's index from x and y values
	    // also does range checking
	    inline unsigned int pos(int x, int y) {
		if (x >= width || y >= height || x < 0 || y < 0) {
		    return width*height;
		}
		return y*width + x;
	    }
    };

    const int defwidth = 600;
    const int defheight = 410;
}

#endif /* FIELD_H_INCLUDED */
