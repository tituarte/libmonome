/*
 * Copyright (c) 2007-2010, William Light <will@visinin.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * life.c
 * conway's game of life
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <monome.h>

#define COLUMNS     16
#define ROWS        16

typedef struct cell cell_t;

struct cell {
	uint alive;
	uint mod_next;

	uint x;
	uint y;

	cell_t *neighbors[8];
	uint nnum;
};

cell_t world[ROWS][COLUMNS];
monome_t *monome;

static void chill(int msec) {
	struct timespec rem, req;

	req.tv_nsec  = msec * 1000000;
	req.tv_sec   = req.tv_nsec / 1000000000;
	req.tv_nsec -= req.tv_sec * 1000000000;

	nanosleep(&req, &rem);
}

static void handle_press(const monome_event_t *e, void *user_data) {
	world[e->x][e->y].mod_next = 1;
}

static void mod_neighbors(cell_t *c, int delta) {
	int i;

	for( i = 0; i < 8; i++ )
		c->neighbors[i]->nnum += delta;
}

static void exit_on_signal(int s) {
	exit(EXIT_SUCCESS);
}

static void init_world() {
	uint x, y;
	cell_t *c;

	for( x = 0; x < COLUMNS; x++ ) {
		for( y = 0; y < ROWS; y++ ) {
			c = &world[x][y];

			c->mod_next = 0;
			c->alive    = 0;
			c->nnum     = 0;
			c->x        = x;
			c->y        = y;

			c->neighbors[0] = &world[(x - 1) % COLUMNS][(y - 1) % ROWS];
			c->neighbors[1] = &world[(x - 1) % COLUMNS][(y + 1) % ROWS];
			c->neighbors[2] = &world[(x - 1) % COLUMNS][y];

			c->neighbors[3] = &world[(x + 1) % COLUMNS][(y - 1) % ROWS];
			c->neighbors[4] = &world[(x + 1) % COLUMNS][(y + 1) % ROWS];
			c->neighbors[5] = &world[(x + 1) % COLUMNS][y];

			c->neighbors[6] = &world[x][(y - 1) % ROWS];
			c->neighbors[7] = &world[x][(y + 1) % ROWS];
		}
	}
}

static void close_monome() {
	monome_clear(monome, MONOME_CLEAR_OFF);
	monome_close(monome);
}

int main(int argc, char **argv) {
	uint x, y;
	int tick = 0;

	cell_t *c;

	if( !(monome = monome_open("osc.udp://127.0.0.1:8080/life", "8000")) )
		return EXIT_FAILURE;

	signal(SIGINT, exit_on_signal);
	atexit(close_monome);

	monome_register_handler(monome, MONOME_BUTTON_DOWN, handle_press, NULL);

	init_world();
	monome_clear(monome, MONOME_CLEAR_OFF);

	while(1) {
		tick++;
		if( !(tick %= 3) )
			while( monome_event_handle_next(monome) );

		for( x = 0; x < COLUMNS; x++ ) {
			for( y = 0; y < ROWS; y++ ) {
				c = &world[x][y];

				if( c->mod_next ) {
					if( c->alive ) {
						c->alive = 0;
						mod_neighbors(c, -1);

						monome_led_off(monome, x, y);
					} else {
						c->alive = 1;
						mod_neighbors(c, 1);

						monome_led_on(monome, x, y);
					}

					c->mod_next = 0;
				}
			}
		}

		for( x = 0; x < COLUMNS; x++ ) {
			for( y = 0; y < ROWS; y++ ) {
				c = &world[x][y];

				switch( c->nnum ) {
				case 3:
					if( !c->alive )
						c->mod_next = 1;

				case 2:
					break;

				default:
					if( c->alive )
						c->mod_next = 1;

					break;
				}
			}
		}

		chill(50);
	}

	return EXIT_SUCCESS;
}
