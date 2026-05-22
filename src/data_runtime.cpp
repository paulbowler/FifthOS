#include "data_runtime.h"

#include <time.h>

#include "config.h"
#include "forth_runtime.h"
#include "network.h"

namespace {

constexpr time_t MIN_VALID_UNIX = 946684800; // 2000-01-01T00:00:00Z
constexpr uint32_t TIME_RESYNC_MS = 60000;
constexpr int MAX_TASKS = 8;

struct Task {
    bool used;
    bool active;
    bool repeating;
    uint32_t intervalMs;
    uint32_t nextAt;
    long xt;
};

Task tasks[MAX_TASKS + 1] = {};
bool stationSeen = false;
bool timeConfigured = false;
uint32_t lastSyncAttempt = 0;

bool validTask(int16_t task)
{
    return task > 0 && task <= MAX_TASKS && tasks[task].used;
}

bool unixTimeValid(time_t value)
{
    return value >= MIN_VALID_UNIX;
}

void startTimeSync()
{
    configTzTime(timeZone, "pool.ntp.org", "time.nist.gov", "time.google.com");
    timeConfigured = true;
    lastSyncAttempt = millis();
}

void callTaskXt(long xt)
{
    if (xt == 0) {
        return;
    }

    char buffer[48];
    snprintf(buffer, sizeof(buffer), "%ld EXECUTE", xt);
    forth_eval_text(buffer);
}

} // namespace

void data_runtime_init()
{
    setenv("TZ", timeZone, 1);
    tzset();
}

void data_runtime_tick()
{
    bool connected = networkIsStationConnected();
    if (connected && !stationSeen) {
        startTimeSync();
    }
    stationSeen = connected;

    if (connected && !data_time_synced() && (!timeConfigured || millis() - lastSyncAttempt >= TIME_RESYNC_MS)) {
        startTimeSync();
    }

    uint32_t now = millis();
    for (int16_t i = 1; i <= MAX_TASKS; ++i) {
        Task& task = tasks[i];
        if (!task.used || !task.active || task.xt == 0) {
            continue;
        }
        if (static_cast<int32_t>(now - task.nextAt) < 0) {
            continue;
        }

        if (task.repeating && task.intervalMs > 0) {
            task.nextAt = now + task.intervalMs;
        } else {
            task.active = false;
        }

        callTaskXt(task.xt);
    }
}

bool data_time_synced()
{
    return unixTimeValid(time(nullptr));
}

long data_time_unix()
{
    time_t now = time(nullptr);
    return unixTimeValid(now) ? static_cast<long>(now) : 0;
}

void data_time_hms(int* hour, int* minute, int* second)
{
    time_t now = time(nullptr);
    struct tm localTm = {};
    if (unixTimeValid(now) && localtime_r(&now, &localTm)) {
        *hour = localTm.tm_hour;
        *minute = localTm.tm_min;
        *second = localTm.tm_sec;
        return;
    }
    *hour = 0;
    *minute = 0;
    *second = 0;
}

void data_time_ymd(int* year, int* month, int* day)
{
    time_t now = time(nullptr);
    struct tm localTm = {};
    if (unixTimeValid(now) && localtime_r(&now, &localTm)) {
        *year = localTm.tm_year + 1900;
        *month = localTm.tm_mon + 1;
        *day = localTm.tm_mday;
        return;
    }
    *year = 0;
    *month = 0;
    *day = 0;
}

int data_time_wday()
{
    time_t now = time(nullptr);
    struct tm localTm = {};
    if (unixTimeValid(now) && localtime_r(&now, &localTm)) {
        return localTm.tm_wday;
    }
    return 0;
}

unsigned long data_millis_now()
{
    return millis();
}

int16_t data_task_new()
{
    for (int16_t i = 1; i <= MAX_TASKS; ++i) {
        if (!tasks[i].used) {
            tasks[i] = {};
            tasks[i].used = true;
            return i;
        }
    }
    return 0;
}

void data_task_every(int16_t task, uint32_t intervalMs, long xt)
{
    if (!validTask(task)) {
        return;
    }
    tasks[task].intervalMs = intervalMs == 0 ? 1 : intervalMs;
    tasks[task].xt = xt;
    tasks[task].repeating = true;
    tasks[task].nextAt = millis() + tasks[task].intervalMs;
}

void data_task_once(int16_t task, uint32_t intervalMs, long xt)
{
    if (!validTask(task)) {
        return;
    }
    tasks[task].intervalMs = intervalMs;
    tasks[task].xt = xt;
    tasks[task].repeating = false;
    tasks[task].nextAt = millis() + intervalMs;
}

void data_task_start(int16_t task)
{
    if (!validTask(task)) {
        return;
    }
    tasks[task].active = true;
    tasks[task].nextAt = millis() + tasks[task].intervalMs;
}

void data_task_stop(int16_t task)
{
    if (!validTask(task)) {
        return;
    }
    tasks[task].active = false;
}

bool data_task_active(int16_t task)
{
    return validTask(task) && tasks[task].active;
}
