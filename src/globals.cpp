#include <Arduino.h>

#include "globals.h"

long* Pointer;
long WP, top, len;
long long int d, n, m;

uint8_t forthCallDepth = 0;
LocalFrame forthLocalFrames[32] = {};

bool forthLocalsCompileActive = false;
uint8_t forthLocalsCompileCount = 0;
uint8_t forthLocalsCompileInputCount = 0;
long forthLocalsSavedCp = 0;
long forthLocalsSavedLast = 0;
long forthLocalsSavedContext = 0;

long forthWordLocalsEnter = 0;
long forthWordLocalFetch = 0;
long forthWordLocalSet = 0;
long forthWordLocalsEnd = 0;
long forthWordScomp = 0;
