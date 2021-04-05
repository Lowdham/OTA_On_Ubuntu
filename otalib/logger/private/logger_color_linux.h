#ifndef LOGGER_COLOR_LINUX_H
#define LOGGER_COLOR_LINUX_H

#include <string>

namespace otalib {
namespace color {
enum class ForegroundColor {
    None = 0,
    Black = 30,
    Red = 31,
    Green = 32,
    Orange = 33,
    Yellow = 33,
    Blue = 34,
    Solferino = 35,
    Cyan = 36,
    LightGray = 37
};
enum class BackgroundColor {
    None = 0,
    Black = 40,
    Red = 41,
    Green = 42,
    Orange = 43,
    Blue = 44,
    Solferino = 45,
    Cyan = 46,
    LightGray = 47,
    DeepGray = 100,
    LightRed = 101,
    LightGreen = 102,
    Yellow = 103,
    LightBlue = 104,
    LightPurple = 105,
    BlueGreen = 106,
    White = 107
};

enum class ControlCode {
    None = 0,
    Highlight = 1,
    LessHighlight = 2,
    Italic = 3,
    UnderLine = 4,
};

}  // namespace color
using fgColor = color::ForegroundColor;
using bgColor = color::BackgroundColor;
using ctlColor = color::ControlCode;

class logger_color {
   public:
    explicit logger_color(fgColor fgcolor = fgColor::None,
                          bgColor bgcolor = bgColor::None,
                          ctlColor ctlcolor = ctlColor::None) {
        prefix_.append("\033[");
        bool last_ = false;
        if (fgcolor != fgColor::None) {
            last_ = true;
            prefix_.append(std::to_string(static_cast<int>(fgcolor)));
        }
        if (bgcolor != bgColor::None) {
            if (last_) prefix_.append(";");
            prefix_.append(std::to_string(static_cast<int>(bgcolor)));
            last_ = true;
        }
        if (ctlcolor != ctlColor::None) {
            if (last_) prefix_.append(";");
            prefix_.append(std::to_string(static_cast<int>(ctlcolor)));
            last_ = true;
        }
        if (last_) prefix_.append("m");
    }

    std::string operator()(const std::string &text) {
        std::string value = prefix_;
        value.append(text);
        value.append("\033[0m");
        return value;
    }

   private:
    std::string prefix_;
};
}
#endif
