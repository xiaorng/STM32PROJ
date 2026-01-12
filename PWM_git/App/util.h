
// App/util.h
#pragma once
#include <stdint.h>

static inline int clampi(int v, int lo, int hi){
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}
