#pragma once
#include <cstdint>
bool gowAndroidPaused();
bool gowAndroidAsyncFinish();
void gowAndroidReadPad(uint8_t *data);
void gowAndroidFrame();
void gowAndroidMovieFrame();
void gowAndroidMovieEnd();
