/*
 * GTK Cervi
 * vim:set sw=4 sta:
 *
 * Copyright (C) 2003  Tomas Janousek <tomi@nomi.cz>
 * See file COPYRIGHT and COPYING
 */

#pragma implementation

#include "keymap.h"

namespace std {
    // set key k to b state
    void KeyMap::set(unsigned int k, bool b)
    {
	if (b) {
	    for (_v_t::iterator it = _v.begin(); it != _v.end(); it++) {
		if (*it == k)
		    return;
	    }
	    _v.push_back(k);
	} else {
	    for (_v_t::iterator it = _v.begin(); it != _v.end(); it++) {
		if (*it == k) {
		    _v.erase(it);
		    return;
		}
	    }
	}
    }

    // return if key k is pressed
    bool KeyMap::pressed(unsigned int k)
    {
	for (_v_t::iterator it = _v.begin(); it != _v.end(); it++) {
	    if (*it == k)
		return 1;
	}
	return 0;
    }
}
