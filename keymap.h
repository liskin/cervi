/*
 * GTK Cervi
 * vim:set sw=4 sta:
 *
 * Copyright (C) 2003  Tomas Janousek <tomi@nomi.cz>
 * See file COPYRIGHT and COPYING
 */

#ifndef KEYMAP_H_INCLUDED
#define KEYMAP_H_INCLUDED

#pragma interface

#include <vector>

namespace std {
    class KeyMap {
	public:
	    typedef vector<unsigned int> _v_t;
	private:
	    _v_t _v;
	public:
	    void set(unsigned int k, bool b);
	    inline void press(unsigned int k) { set(k,1); }
	    inline void release(unsigned int k) { set(k,0); }

	    bool pressed(unsigned int k);
	    inline bool operator[](unsigned int k) { return pressed(k); }
    };
}

#endif /* KEYMAP_H_INCLUDED */
