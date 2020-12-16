#pragma once
#include "common.h"
#include <chrono>

chrono::high_resolution_clock::time_point startTime;

unsigned elapsed() {
    return chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count();
}
