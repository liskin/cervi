/*
 * GTK Cervi
 * vim:set sw=4 sta:
 *
 * Copyright (C) 2003  Tomas Janousek <tomi@nomi.cz>
 * See file COPYRIGHT and COPYING
 */

#pragma implementation

#include "music.h"
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <esd.h>
#include <sys/stat.h>
#include <math.h>

// from main.cc for accessing highres timer
extern unsigned long long int ticks();

namespace std {
    // array of tone filenames (and esd sample names)
    char *ctones[] = {
	DATADIR "/g.wav",     // 0
	DATADIR "/a.wav",     // 1
	DATADIR "/h.wav",     // 2
	DATADIR "/c.wav",     // 3
	DATADIR "/d.wav",     // 4
	DATADIR "/e.wav",     // 5
	DATADIR "/f.wav",     // 6
	DATADIR "/start.wav", // 7
	DATADIR "/crash.wav"  // 8
    };

    /* music in tones (gahcdef, not cdefgah)
     * taken from Cervi by Vladimir Chvatil, (C) 1993
    char music[] = "c-g-c-g-c-g-cchag-d-g-d-g-d-ggah"
		   "c-g-c-g-c-g-ccdef-c-f-c-f-c-ffed"
		   "c-g-c-g-c-g-cchag-d-g-d-g-d-ggah";*/

    // music in ctones indexes
    char music[] = "73-0-3-0-3-0-33210-4-0-4-0-4-0012"
		    "3-0-3-0-3-0-33456-3-6-3-6-3-6654"
		    "3-0-3-0-3-0-33210-4-0-4-0-4-0012";
    // length of music array
    const int musiclen = sizeof(music) / sizeof(*music);

    // instance of class Music
    Music m;

    // reset to initial state (on new round)
    void Music::reset() {
	mptr = music;
	speed = 0;
    }

    // contructor loads sound files
    Music::Music() : mptr(music), speed(100), play(false), playmusic(true)
    {
	esd = esd_open_sound(0);
	if (esd < 0) return;
	for (int i=0; i<ctoneslen; i++) {
	    // if we did not exit cleanly, samples are left in esd so we can
	    // use them again
	    if ((tones[i] = esd_sample_getid(esd,ctones[i])) >= 0) continue;
	    tones[i] = esd_file_cache(esd,"cervi",ctones[i]);
	}
    }

    // destructor does the cleanup
    Music::~Music()
    {
	if (esd) {
	    for (int i=0; i<ctoneslen; i++)
		if (tones[i] >= 0)
		    esd_sample_free(esd,tones[i]);
	    esd_close(esd);
	}
    }

    // function for music thread
    void Music::thread()
    {
	while (1) {
	    if (play && playmusic) {
		bool mstart = false;

		if (*mptr != '-' && (*mptr - 0x30) >= 0)
		    esd_sample_play(esd,tones[*mptr - 0x30]);
		if (mptr == music)
		    mstart = true;
		mptr++;
		if (*mptr == 0)
		    mptr = music+1;

		unsigned long long int start = ticks();
		while (play) {
		    if (mstart) {
			if ((ticks() - start) > 700)
			break;
		    } else {
			if ((ticks() - start) > (unsigned int)nearbyint(5000/
				(speed-50)))
			break;
		    }
		    usleep(5000);
		}

		if (!play)
		    esd_sample_play(esd,tones[8]);
	    } else {
		// wait for signal
		pause();
	    }
	}
    }
}
