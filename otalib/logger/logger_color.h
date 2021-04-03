#ifndef LOGGER_COLOR_H
#define LOGGER_COLOR_H

#ifdef __linux__
#include "private/logger_color_linux.h"
#define LOGGER_COLOR(FG, BG) logger_color((FG), (BG), ctlColor::Highlight)
#define LOGGER_COLOR_RESET
#else
#include "private/logger_color_win.h"
#define LOGGER_COLOR(FG, BG) logger_color((FG), (BG), true)
#define LOGGER_COLOR_RESET logger_color::reset();
#endif

#define __LOGGER_COLOR 1

#endif
