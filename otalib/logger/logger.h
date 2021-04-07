#ifndef LOGGER_H
#define LOGGER_H

#include <time.h>

#include <QString>
#include <iostream>
#include <sstream>

#include "../utils.hpp"
#include "logger_color.h"

namespace otalib {

static constexpr char kHeadTagFatalError[] = "[FATAL ERROR]";
static constexpr char kHeadTagError[] = "[FATAL ERROR]";
static constexpr char kHeadTagDebug[] = "[DEBUG]";
static constexpr char kHeadTagWarn[] = "[WARNNING]";
static constexpr char kHeadTagInfo[] = "[INFO]";
static constexpr char kHeadTagSuccess[] = "[SUCCESS]";

template <auto HeadTag, auto Sep, fgColor fcolor, bool otime = false,
          bool linefeed = true, bool thread_safe = false>
struct PrintCtrl {
  static constexpr decltype(auto) tag() noexcept { return HeadTag; }
  static constexpr decltype(auto) sep() noexcept { return Sep; }
  static constexpr decltype(auto) color() noexcept { return fcolor; }
  static constexpr bool time() noexcept { return otime; }
  static constexpr bool isLinefeed() noexcept { return linefeed; }
  static constexpr bool isThreadSafe() noexcept { return thread_safe; }
};

using GeneralFerrorCtrl = PrintCtrl<kHeadTagFatalError, ' ', fgColor::Red>;
using GeneralErrorCtrl = PrintCtrl<kHeadTagError, ' ', fgColor::Red>;
using GeneralDebugCtrl = PrintCtrl<kHeadTagDebug, ' ', fgColor::Yellow>;
using GeneralWarnCtrl = PrintCtrl<kHeadTagWarn, ' ', fgColor::Yellow>;
using GeneralInfoCtrl = PrintCtrl<kHeadTagInfo, ' ', fgColor::None>;
using GeneralSuccessCtrl = PrintCtrl<kHeadTagSuccess, ' ', fgColor::Green>;

template <typename ctrl, typename OutputStream, typename... Args>
void print(OutputStream&& out, Args&&... args) {
  static ::std::stringstream inner_stream;
  static SpinLock slock;

  if constexpr (ctrl::time()) {
    char buf[64]{0};
    time_t t = time(nullptr);
    strftime(buf, 64, "%Y-%m-%d %H:%M:%S", localtime(&t));
    std::string head = std::string(buf) + ctrl::sep();
    {
      if constexpr (ctrl::isThreadSafe()) SpinLock::Acquire locker(slock);
      inner_stream << head;
    }
  }

  auto outWithSep = [&](auto& stream, const auto& args) {
    stream << ctrl::sep() << args;
  };

  {
    if constexpr (ctrl::isThreadSafe()) SpinLock::Acquire locker(slock);
    inner_stream << ctrl::tag();
    (..., outWithSep(inner_stream, args));
  }
  // Put all the msg into out.
  std::string msg =
      LOGGER_COLOR(ctrl::color(), bgColor::None).operator()(inner_stream.str());

  // Clear the str.
  inner_stream.str("");

  // May need support for customized endl;
  {
    if constexpr (ctrl::isThreadSafe()) SpinLock::Acquire locker(slock);
    out << msg;
    if constexpr (ctrl::isLinefeed())
      out << '\n';
    else
      out << std::flush;
  }
  LOGGER_COLOR_RESET
}

}  // namespace otalib

inline std::ostream& operator<<(std::ostream& out, const QString& str) {
  out << str.toStdString();
  return out;
}

inline std::ostream& operator<<(std::ostream& out, QString&& str) {
  out << str.toStdString();
  return out;
}

#endif
