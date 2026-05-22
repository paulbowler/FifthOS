#pragma once

#include <Arduino.h>

void data_runtime_init();
void data_runtime_tick();

bool data_time_synced();
long data_time_unix();
void data_time_hms(int* hour, int* minute, int* second);
void data_time_ymd(int* year, int* month, int* day);
int data_time_wday();
unsigned long data_millis_now();

int16_t data_task_new();
void data_task_every(int16_t task, uint32_t intervalMs, long xt);
void data_task_once(int16_t task, uint32_t intervalMs, long xt);
void data_task_start(int16_t task);
void data_task_stop(int16_t task);
bool data_task_active(int16_t task);
