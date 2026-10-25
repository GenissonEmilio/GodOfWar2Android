#pragma once
#include <cstdint>
bool gowAndroidPaused();
void gowAndroidReadPad(uint8_t *data);
void gowAndroidFrame();
void gowAndroidMovieFrame();
void gowAndroidMovieEnd();
