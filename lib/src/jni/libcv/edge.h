/* edge.h: rudimentary edge detector
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * All rights reserved.
 */

#pragma once

static uint8_t edge_[4];

#define DEBUG_EDGE

#ifndef DEBUG_EDGE
#define store_edge() ;
#define read_edge(edge) ;
#else
static inline void read_edge(uint8_t edge[4])
{
	edge[0] = edge_[0];
	edge[1] = edge_[1];
	edge[2] = edge_[2];
	edge[3] = edge_[3];
}
#define store_edge() {\
	edge_[0] = c0;\
	edge_[1] = c1;\
	edge_[2] = c2;\
	edge_[3] = c3;\
}
#endif

#if 0
#define fall_condition()\
	((c0 - c1) >= 0 && (c2 - c3) >= 0 && (c1 - c2) > MIN_FALL_SLOPE)
#else
#define fall_condition()\
	(c0 - c1) > MIN_FALL_SLOPE &&\
	(c2 - c3) > MIN_FALL_SLOPE &&\
	(c1 - c2) > MIN_FALL_SLOPE
#endif

/*
 *
 * c0 -- c1
 *        \
 *         \ <-- slope
 *          \
 *           c2 -- c3
 *           ^-----^
 *            foot width
 *
 * */

#define MIN_FALL_SLOPE 2
#define FALL_FOOT_WIDTH 2
#define FALL_ADVANCE (FALL_FOOT_WIDTH + FALL_FOOT_WIDTH)

static inline uint8_t falling_edge(uint16_t *x, uint16_t y)
{
	uint8_t c0 = byte(*x, y);
	uint8_t c1 = byte(*x + 1, y);
	uint8_t c2 = byte(*x + FALL_FOOT_WIDTH, y);
	uint8_t c3 = byte(*x + FALL_FOOT_WIDTH + 1, y);

	*x += FALL_ADVANCE;
	store_edge();

	if (fall_condition())
		return 1;

	return 0;
}

static inline uint8_t falling_edge_right(uint16_t x, uint16_t y)
{
	uint8_t c0 = byte(x, y);
	uint8_t c1 = byte(x + 1, y);
	uint8_t c2 = byte(x + FALL_FOOT_WIDTH, y);
	uint8_t c3 = byte(x + FALL_FOOT_WIDTH + 1, y);

	store_edge();

	if (fall_condition())
		return 1;

	return 0;
}

/*
 *
 *           c1 -- c0
 *           /
 *          / <-- slope
 *         /
 * c3 -- c2
 * ^-----^
 *  foot width
 *
 * */

static inline uint8_t falling_edge_left(uint16_t x, uint16_t y)
{
	uint8_t c0 = byte(x, y);
	uint8_t c1 = byte(x - 1, y);
	uint8_t c2 = byte(x - FALL_FOOT_WIDTH, y);
	uint8_t c3 = byte(x - FALL_FOOT_WIDTH - 1, y);

	store_edge();

	if (fall_condition())
		return 1;

	return 0;
}

/*
 *
 *  c3
 *  |
 *  c2
 *   \
 *    \ <-- slope
 *     \
 *      c1
 *      |  foot width
 *      c0
 *
 * */

static inline uint8_t falling_edge_up(uint16_t x, uint16_t y)
{
	uint8_t c0 = byte(x, y);
	uint8_t c1 = byte(x, y - 1);
	uint8_t c2 = byte(x, y - FALL_FOOT_WIDTH);
	uint8_t c3 = byte(x, y - FALL_FOOT_WIDTH - 1);

	store_edge();

	if (fall_condition())
		return 1;

	return 0;
}

/*
 *
 *  c0
 *  |
 *  c1
 *   \
 *    \ <-- slope
 *     \
 *      c2
 *      |  foot width
 *      c3
 *
 * */

static inline uint8_t falling_edge_down(uint16_t x, uint16_t y)
{
	uint8_t c0 = byte(x, y);
	uint8_t c1 = byte(x, y + 1);
	uint8_t c2 = byte(x, y + FALL_FOOT_WIDTH);
	uint8_t c3 = byte(x, y + FALL_FOOT_WIDTH + 1);

	store_edge();

	if (fall_condition())
		return 1;

	return 0;
}

/*
 *
 *           c2 -- c3
 *           /
 *          / <-- slope
 *         /
 * c0 -- c1
 * ^-----^
 *  foot width
 *
 * */

#define MIN_RISE_SLOPE 2
#define RISE_FOOT_WIDTH 2
#define RISE_ADVANCE (RISE_FOOT_WIDTH + RISE_FOOT_WIDTH)

#if 0
#define rise_condition()\
	((c1 - c0) >= 0 && (c3 - c2) >= 0 && (c2 - c1) > MIN_RISE_SLOPE) &&\
		((c3 - c2 - c1) < c0)
#else
//#define rise_condition() (c3 + c2) - (c1 + c0) > 0
#define rise_condition()\
	(c1 - c0) > MIN_RISE_SLOPE &&\
	(c3 - c2) > MIN_RISE_SLOPE &&\
	(c2 - c1) > MIN_RISE_SLOPE
#endif

static inline uint8_t rising_edge(uint16_t *x, uint16_t y)
{
	uint8_t c0 = byte(*x, y);
	uint8_t c1 = byte(*x + 1, y);
	uint8_t c2 = byte(*x + RISE_FOOT_WIDTH, y);
	uint8_t c3 = byte(*x + RISE_FOOT_WIDTH + 1, y);

	*x += RISE_ADVANCE;
	store_edge();

	if (rise_condition())
		return 1;

	return 0;
}

static inline uint8_t rising_edge_right(uint16_t x, uint16_t y)
{
	uint8_t c0 = byte(x, y);
	uint8_t c1 = byte(x + 1, y);
	uint8_t c2 = byte(x + RISE_FOOT_WIDTH, y);
	uint8_t c3 = byte(x + RISE_FOOT_WIDTH + 1, y);
	uint8_t slope = MIN_RISE_SLOPE;

	store_edge();

	if (rise_condition())
		return 1;

	return 0;
}

/*
 *
 * c3 -- c2
 *        \
 *         \ <-- slope
 *          \
 *           c1 -- c0
 *           ^-----^
 *            foot width
 * */

static inline uint8_t rising_edge_left(uint16_t x, uint16_t y)
{
	uint8_t c0 = byte(x, y);
	uint8_t c1 = byte(x - 1, y);
	uint8_t c2 = byte(x - RISE_FOOT_WIDTH, y);
	uint8_t c3 = byte(x - RISE_FOOT_WIDTH - 1, y);
	uint8_t slope = MIN_RISE_SLOPE;

	store_edge();

	if (rise_condition())
		return 1;

	return 0;
}

/*
 *
 *  c3
 *  |
 *  c2
 *   \
 *    \ <-- slope
 *     \
 *      c1
 *      |  foot width
 *      c0
 *
 * */

static inline uint8_t rising_edge_up(uint16_t x, uint16_t y)
{
	uint8_t c0 = byte(x, y);
	uint8_t c1 = byte(x, y - 1);
	uint8_t c2 = byte(x, y - RISE_FOOT_WIDTH);
	uint8_t c3 = byte(x, y - RISE_FOOT_WIDTH - 1);
	uint8_t slope = MIN_RISE_SLOPE;

	store_edge();

	if (rise_condition())
		return 1;

	return 0;
}

/*
 *
 *  c0
 *  |
 *  c1
 *   \
 *    \ <-- slope
 *     \
 *      c2
 *      |  foot width
 *      c3
 *
 * */

static inline uint8_t rising_edge_down(uint16_t x, uint16_t y)
{
	uint8_t c0 = byte(x, y);
	uint8_t c1 = byte(x, y + 1);
	uint8_t c2 = byte(x, y + RISE_FOOT_WIDTH);
	uint8_t c3 = byte(x, y + RISE_FOOT_WIDTH + 1);
	uint8_t slope = MIN_RISE_SLOPE;

	store_edge();

	if (rise_condition())
		return 1;

	return 0;
}
