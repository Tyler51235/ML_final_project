#pragma once
#include <cstddef>

// Dummy model so the project compiles.
// We are not actually using it yet in main.cpp.

extern const unsigned char g_model[];
extern const size_t g_model_len;

const unsigned char g_model[] = { 0x00 };
const size_t g_model_len = sizeof(g_model);
