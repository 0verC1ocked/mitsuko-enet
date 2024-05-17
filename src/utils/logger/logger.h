// logger.h

#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>

enum LogLevel {
    EMERGENCY = 0,
    ALERT = 1,
    CRIT = 2,
    ERROR = 3,
    WARNING = 4,
    NOTICE = 5,
    INFO = 6,
    DEBUG = 7,
    MEMORY = 8
};

class Logger {
public:
    static void Log(LogLevel level, const std::string& message, int color = -1);
    static void SetLogLevel(LogLevel level);
    static void printStartMessage();

private:
    static std::string LevelToString(LogLevel level);
    static std::string getTimeStr();
    static int GetColor(LogLevel level);
    static void SetColor(int color);
    static void ResetColor();

    static LogLevel currentLogLevel;
    static char buffer[80];
    static char currentTime[90];
};
