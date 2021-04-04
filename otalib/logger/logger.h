#ifndef LOGGER_H
#define LOGGER_H

#include <time.h>

#include <iostream>
#include <sstream>

#include "logger_color.h"

#define __DEBUG 1
#define __LOGGER_DATETIME 1

namespace logger {

enum class Level {
    K_INFO = 0x01,
    K_ERROR = 0x02,
    K_WARN = 0x03,
    K_DEBUG = 0x04
};

class Logger {
   public:
    static Logger &getStream(Level level) {
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
    void log(const T &val) {
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
            text =
                LOGGER_COLOR(fgColor::Yellow, bgColor::None).operator()(text);
#endif
        if (__normal__ || __debug__) std::cout << text << std::endl;

        LOGGER_COLOR_RESET
    }

    template <class T, class... Args>
    void log(const T &val, Args... args) {
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

#define LOG_LEVEL(LEVEL, ...) \
    logger::Logger::getStream((LEVEL)).log(__VA_ARGS__);
#define LOG_INFO(...) LOG_LEVEL(logger::Level::K_INFO, __VA_ARGS__)
#define LOG_ERROR(...) LOG_LEVEL(logger::Level::K_ERROR, __VA_ARGS__)
#define LOG_WARN(...) LOG_LEVEL(logger::Level::K_WARN, __VA_ARGS__)
#define LOG_DEBUG(...) LOG_LEVEL(logger::Level::K_DEBUG, __VA_ARGS__)

#endif
