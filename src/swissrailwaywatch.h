#pragma once

#include <pebble.h>

// ─── Screen ───────────────────────────────────────────────────────────────────
#define SCREEN_W   144
#define SCREEN_H   168
#define CENTER_X   72
#define CENTER_Y   84
#define RADIUS     65   // outer radius of the dial

// ─── Hand dimensions (Swiss Railway Clock proportions) ────────────────────────
#define HOUR_LEN      38
#define HOUR_TAIL     10
#define HOUR_WIDTH    7

#define MIN_LEN       55
#define MIN_TAIL      12
#define MIN_WIDTH     6

#define SEC_CIRCLE_R  4   // red dot radius at tip of second hand

// ─── Tick dimensions ──────────────────────────────────────────────────────────
#define TICK_MAJOR_LEN  12   // 12 hour indices
#define TICK_MINOR_LEN  5    // 60 minute indices
#define TICK_MAJOR_W    4
#define TICK_MINOR_W    2

// ─── Day/date window ──────────────────────────────────────────────────────────
#define DATE_BOX_W  34
#define DATE_BOX_H  28
