#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <sstream>


namespace logger {
    enum class Level {
        INFO = 0x01,
        ERROR = 0x02,
        WARN = 0x03,
        DEBUG = 0x04
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

        // output new line
        void outputMessage() {
            inner_ss_ << std::endl;
            if (level_ == Level::ERROR || level_ == Level::DEBUG)
                std::cerr << inner_ss_.str();
            else
                std::cout << inner_ss_.str();
        }

        template<class T>
        void outputMessage(const T &val) {
            appendTag();
            inner_ss_ << val << std::endl;;
            if (level_ == Level::ERROR || level_ == Level::DEBUG)
                std::cerr << inner_ss_.str();
            else
                std::cout << inner_ss_.str();
        }

        template<class T, class...Args>
        void outputMessage(const T &val, Args...args) {
            appendTag();
            inner_ss_ << val << " ";
            outputMessage(args...);
        }

    private:
        void appendTag() {
            if (!first_) {
                std::string level_label;
                switch (level_) {
                    case Level::INFO:
                        level_label = "[INFO]";
                        break;
                    case Level::ERROR:
                        level_label = "[ERROR]";
                        break;
                    case Level::WARN:
                        level_label = "[WARN]";
                        break;
                    case Level::DEBUG:
                        level_label = "[DEBUG]";
                        break;
                    default:
                        level_label = "[DEFAULT]";
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
}

#define LOG_LEVEL(LEVEL, ...) logger::Logger::getStream((LEVEL)).outputMessage(__VA_ARGS__);

#define LOG_INFO(...) LOG_LEVEL(logger::Level::INFO,__VA_ARGS__)
#define LOG_ERROR(...) LOG_LEVEL(logger::Level::ERROR,__VA_ARGS__)
#define LOG_WARN(...) LOG_LEVEL(logger::Level::WARN,__VA_ARGS__)
#define LOG_DEBUG(...) LOG_LEVEL(logger::Level::DEBUG,__VA_ARGS__)


#endif



