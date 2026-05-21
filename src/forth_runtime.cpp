#include "forth_runtime.h"

#include "globals.h"
#include "util.h"
#include "vm.h"

namespace {

constexpr size_t FORTH_EVAL_ADDR = 0x8000;
constexpr size_t FORTH_EVAL_MAX = 0x7000;

}

void forth_eval_buffer(const char* text, size_t len)
{
    if (!text) {
        return;
    }

    if (len >= FORTH_EVAL_MAX) {
        len = FORTH_EVAL_MAX - 1;
    }

    memcpy(cData + FORTH_EVAL_ADDR, text, len);
    cData[FORTH_EVAL_ADDR + len] = 0;

    data[0x66] = 0; // >IN
    data[0x67] = static_cast<long>(len); // #TIB
    data[0x68] = FORTH_EVAL_ADDR; // 'TIB
    P = 0x180; // EVAL
    WP = 0x184;
    evaluate();
}

void forth_eval_text(const char* text)
{
    forth_eval_buffer(text, strlen(text));
}
