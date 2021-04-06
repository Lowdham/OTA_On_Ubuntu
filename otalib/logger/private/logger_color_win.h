#ifndef LOGGER_COLOR_WIN_H
#define LOGGER_COLOR_WIN_H

#include <windows.h>

#include <iostream>
#include <string>

namespace otalib {
namespace color {
enum class FgColor {
  None = 0,
  Red = FOREGROUND_RED,
  Green = FOREGROUND_GREEN,
  Blue = FOREGROUND_BLUE,
  White = Red | Green | Blue,
  Black = ~White,
  Yellow = Red | Green,
  Other1 = Red | Blue,
  Other2 = Green | Blue
};

enum class BgColor {
  None = 0,
  Red = BACKGROUND_RED,
  Green = BACKGROUND_GREEN,
  Blue = BACKGROUND_BLUE,
  White = Red | Green | Blue,
  Black = ~White,
  Yellow = Red | Green,
  Other1 = Red | Blue,
  Other2 = Green | Blue
};

}  // namespace color
using fgColor = color::FgColor;
using bgColor = color::BgColor;

class logger_color {
 public:
  explicit logger_color(fgColor fgcolor, bgColor bgcolor, bool highlight = true)
      : newAttr_{0} {
    handle_ = ::GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO scbi;
    ::GetConsoleScreenBufferInfo(handle_, &scbi);
    oldAttr_ = scbi.wAttributes;

    if (highlight) {
      newAttr_ |= FOREGROUND_INTENSITY;
    }

    setFg(fgcolor);
    setBg(bgcolor);
    // newAttr_ |= BACKGROUND_INTENSITY;
  }

  std::string operator()(const std::string &text) { return text; }
  static void reset() {
    SetConsoleTextAttribute(::GetStdHandle(STD_OUTPUT_HANDLE), oldAttr_);
  }

 private:
  void setFg(fgColor foreground_color) {
    newAttr_ |= static_cast<WORD>(foreground_color);
    SetConsoleTextAttribute(handle_, newAttr_);
  }

  void setBg(bgColor background_color) {
    newAttr_ |= static_cast<WORD>(background_color);
    SetConsoleTextAttribute(handle_, newAttr_);
  }

  void setV(WORD color_value) {
    newAttr_ = (WORD)color_value;
    SetConsoleTextAttribute(handle_, newAttr_);
  }

 private:
  HANDLE handle_;
  static inline WORD oldAttr_ = 0;
  WORD newAttr_;
};
}  // namespace otalib
#endif
