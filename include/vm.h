#pragma once

#include <Arduino.h>

#define vmPop top = stack[(unsigned char)S--]
#define vmPush                       \
    stack[(unsigned char)++S] = top; \
    top =
#define vmPopR rack[(unsigned char)R--]
#define vmPushR rack[(unsigned char)++R]

extern long rack[];
extern long stack[];

extern long P;
extern long IP;
extern long data[];
extern unsigned char R;
extern unsigned char S;
extern long links;
extern uint8_t* cData;
