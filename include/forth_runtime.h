#pragma once

#include <Arduino.h>

void forth_eval_text(const char* text);
void forth_eval_buffer(const char* text, size_t len);
