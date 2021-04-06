#ifndef LOGGER_H
#define LOGGER_H

#include <time.h>

#include <QString>
#include <iostream>
#include <sstream>

#include "logger_color.h"
#include "utils.hpp"

#define __DEBUG 1
#define __LOGGER_DATETIME 1

namespace otalib {
namespace logger {

enum class Level {
  K_INFO = 0x01,
  K_ERROR = 0x02,
  K_WARN = 0x03,
  K_DEBUG = 0x04
};

class Logger {
 public:
  static Logger& getStream(Level level) {
    static Logger logger;
    logger.inner_ss_.str("");
    logger.level_ = level;
    logger.first_ = false;
    std::cout.clear();
    std::cout.flush();
    return logger;
  };

  void log() { std::cout << std::endl; }

  template <class T>
  void log(const T& val) {
    appendTag();
    inner_ss_ << val;
    bool __normal__ = true, __debug__ = false;
    std::string text = inner_ss_.str();
#ifdef __LOGGER_COLOR
    if (level_ == Level::K_ERROR)
      text = LOGGER_COLOR(fgColor::Red, bgColor::None).operator()(text);
    else if (level_ == Level::K_DEBUG) {
      text = LOGGER_COLOR(fgColor::Red, bgColor::None).operator()(text);
      __normal__ = false;
#ifdef __DEBUG
      __debug__ = true;
#endif
    } else if (level_ == Level::K_INFO)
      text = LOGGER_COLOR(fgColor::Green, bgColor::None).operator()(text);
    else if (level_ == Level::K_WARN)
      text = LOGGER_COLOR(fgColor::Yellow, bgColor::None).operator()(text);
#endif
    if (__normal__ || __debug__) std::cout << text << std::endl;
    // restore the attribute of console on Windows
    LOGGER_COLOR_RESET
  }

  template <class T, class... Args>
  void log(const T& val, Args... args) {
    appendTag();
    inner_ss_ << val << " ";
    log(args...);
  }

 private:
  void appendTag() {
    if (!first_) {
      std::string level_label;

#ifdef __LOGGER_DATETIME
      char buf[64]{0};
      time_t t = time(nullptr);
      strftime(buf, 64, "%Y-%m-%d %H:%M:%S", localtime(&t));
      level_label = std::string(buf) + " ";
#endif

      switch (level_) {
        case Level::K_INFO:
          level_label += "[INFO]";
          break;
        case Level::K_ERROR:
          level_label += "[ERROR]";
          break;
        case Level::K_WARN:
          level_label += "[WARN]";
          break;
        case Level::K_DEBUG:
          level_label += "[DEBUG]";
          break;
        default:
          level_label += "[DEFAULT]";
          break;
      }

      inner_ss_ << level_label << " ";
      first_ = true;
    }
  }

 private:
  std::stringstream inner_ss_;
  bool first_;
  Level level_;
};
}  // namespace logger

static constexpr char kHeadTagFatalError[] = "[FATAL ERROR]";
static constexpr char kHeadTagDebug[] = "[DEBUG]";

template <auto HeadTag, auto Sep, fgColor fcolor,
          bool otime = true, bool linefeed = true, bool thread_safe = false>
struct PrintCtrl {
  static constexpr decltype(auto) tag() noexcept { return HeadTag; }
  static constexpr decltype(auto) sep() noexcept { return Sep; }
  static constexpr decltype(auto) color() noexcept { return fcolor; }
  static constexpr bool time() noexcept { return otime; }
  static constexpr bool isLinefeed() noexcept { return linefeed; }
  static constexpr bool isThreadSafe() noexcept { return thread_safe; }
};

using GeneralDebugCtrl = PrintCtrl<kHeadTagDebug, ' ', fgColor::Red, true>;

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

  // May need support for customized endl;
  {
    if constexpr (ctrl::isThreadSafe()) SpinLock::Acquire locker(slock);
    out << msg << std::endl;
    if constexpr (ctrl::isLinefeed()) {
      out << '\n';
    }
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

#define LOG_LEVEL(LEVEL, ...) \
  otalib::logger::Logger::getStream((LEVEL)).log(__VA_ARGS__);
#define LOG_INFO(...) LOG_LEVEL(otalib::logger::Level::K_INFO, __VA_ARGS__)
#define LOG_ERROR(...) LOG_LEVEL(otalib::logger::Level::K_ERROR, __VA_ARGS__)
#define LOG_WARN(...) LOG_LEVEL(otalib::logger::Level::K_WARN, __VA_ARGS__)
#define LOG_DEBUG(...) LOG_LEVEL(otalib::logger::Level::K_DEBUG, __VA_ARGS__)

#endif
