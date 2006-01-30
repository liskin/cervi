/*
 * GTK Cervi
 * vim:set sw=4 sta:
 *
 * Copyright (C) 2003  Tomas Janousek <tomi@nomi.cz>
 * See file COPYRIGHT and COPYING
 */

#ifndef MUSIC_H_INCLUDED
#define MUSIC_H_INCLUDED

#pragma interface

#include <esd.h>
#include <cstdio>

namespace std {
    // count of ctones
    const int ctoneslen = 9;
    
    class Music {
	private:
	    int esd;
	    int tones[ctoneslen];
	    char *mptr;

	public:
	    volatile double speed;
	    volatile bool play;

	    Music();
	    ~Music();
	    void thread();
	    void reset();
    };
    extern Music m;
}

#endif /* MUSIC_H_INCLUDED */
