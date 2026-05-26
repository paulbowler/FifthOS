#pragma once

#include <Arduino.h>

extern long* Pointer;
extern long WP, top, len;
extern long long int d, n, m;

struct LocalFrame {
    bool active;
    uint8_t count;
    uint8_t returnSlot;
};

extern uint8_t forthCallDepth;
extern LocalFrame forthLocalFrames[32];

extern bool forthLocalsCompileActive;
extern uint8_t forthLocalsCompileCount;
extern uint8_t forthLocalsCompileInputCount;
extern long forthLocalsSavedCp;
extern long forthLocalsSavedLast;
extern long forthLocalsSavedContext;

extern long forthWordLocalsEnter;
extern long forthWordLocalFetch;
extern long forthWordLocalSet;
extern long forthWordLocalsEnd;
extern long forthWordScomp;
