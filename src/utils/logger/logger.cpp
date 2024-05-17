// logger.cpp

#include "../../server.h"

LogLevel Logger::currentLogLevel = INFO;

std::string Logger::getTimeStr() {
    char buffer[80] = "";
    char currentTime[90] = "";
    timeval curTime;
    gettimeofday(&curTime, NULL);
    int milli = curTime.tv_usec / 1000;
    memset(buffer, 0, sizeof(buffer));
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&curTime.tv_sec));
    memset(currentTime, 0, sizeof(currentTime));
    snprintf(currentTime, sizeof(currentTime), "[%s.%03d] ", buffer, milli);
    return currentTime;
}

void Logger::Log(LogLevel level, const std::string& message, int color) {
  if (level >= MEMORY) {
      return;
  }
  if (color >= 0) {
        SetColor(color);
    } else if (GetColor(level) >= 0) {
        SetColor(GetColor(level));
    }
    std::cout << "[" + LevelToString(level) + "] " + getTimeStr() + message + "\n";

    if (color >= 0 || GetColor(level) >= 0) {
        ResetColor();
    }
}

void Logger::SetLogLevel(LogLevel level) {
    currentLogLevel = level;
}

std::string Logger::LevelToString(LogLevel level) {
    switch (level) {
    case EMERGENCY: return "EMERGENCY";
    case ALERT: return "ALERT";
    case CRIT: return "CRIT";
    case ERROR: return "ERROR";
    case WARNING: return "WARNING";
    case NOTICE: return "NOTICE";
    case INFO: return "INFO";
    case DEBUG: return "DEBUG";
    case MEMORY: return "MEMORY";
    default: return "UNKNOWN";
    }
}

int Logger::GetColor(LogLevel level) {
    switch (level) {
    case EMERGENCY: return 0;
    case ALERT: return 1;
    case CRIT: return 2;
    case ERROR: return 3;
    case WARNING: return 4;
    case NOTICE: return 5;
    case INFO: return 6;
    case DEBUG: return 7;
    case MEMORY: return 8;
    default: return 9;
    }
}

void Logger::SetColor(int color) {
    const std::string colorCodes[] = {
        "\x1B[31m", "\x1B[91m", "\x1B[35m", "\x1B[95m",
        "\x1B[33m", "\x1B[93m", "\x1B[32m", "\x1B[92m", "\x1B[32m", "\x1B[97m"
    };

    if (color >= 0 && color < 8) {
        std::cout << colorCodes[color];
    }
}

void Logger::ResetColor() {
    std::cout << "\x1B[0m";
}

void Logger::printStartMessage() {
    std::cout << "▪  ▄▄▄▄▄▄.▄▄ ·   • ▌ ▄ ·. ▪  ▄▄▄▄▄.▄▄ · ▄• ▄▌▄ •▄            ▄▄▄▄· ▪  ▄▄▄▄▄ ▄▄·  ▄ .▄ ▄▄ " << std::endl;
    std::cout << "██ ▀•██ ▀▐█ ▀.   ·██ ▐███▪██ •██  ▐█ ▀. █▪██▌█▌▄▌▪ ▄█▀▄      ▐█ ▀█▪██ •██  ▐█ ▌▪██▪▐█ ██▌" << std::endl;
    std::cout << "▐█·  ▐█.▪▄▀▀▀█▄  ▐█ ▌▐▌▐█·▐█· ▐█.▪▄▀▀▀█▄█▌▐█▌▐▀▀▄·▐█▌.▐▌     ▐█▀▀█▄▐█· ▐█.▪██ ▄▄██▀▀█ ▐█·" << std::endl;
    std::cout << "▐█▌  ▐█▌·▐█▄▪▐█  ██ ██▌▐█▌▐█▌ ▐█▌·▐█▄▪▐█▐█▄█▌▐█.█▌▐█▌.▐▌     ██▄▪▐█▐█▌ ▐█▌·▐███▌██▌▐▀ .▀ " << std::endl;
    std::cout << "▀▀▀  ▀▀▀  ▀▀▀▀   ▀▀  █▪▀▀▀▀▀▀ ▀▀▀  ▀▀▀▀  ▀▀▀ ·▀  ▀ ▀█▄▀▪█    ·▀▀▀▀ ▀▀▀ ▀▀▀ ·▀▀▀ ▀▀▀ ·  ▀ " << std::endl;


    std::cout << "\t\t\t░░░░▄▀▀░░░░░░░░░░░░░░░░░░▀▀▄░░░░" << std::endl;
    std::cout << "\t\t\t░░▄▀░░░░░░░░░░░░░░░░░░░░░░░░▀▄░░" << std::endl;
    std::cout << "\t\t\t░█░░░░░░░░░░░░░░░░░░░░░░░░░░░░█░" << std::endl;
    std::cout << "\t\t\t█░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░█" << std::endl;
    std::cout << "\t\t\t█░░░▄▄██████▄░░░░░░▄██████▄▄░░░█" << std::endl;
    std::cout << "\t\t\t█░░███▀▀█████░░░░░░██▀▀██████░░█" << std::endl;
    std::cout << "\t\t\t▀▄░███▄▄█████░░▄▄░░██▄▄██████░▄▀" << std::endl;
    std::cout << "\t\t\t░▀▄░░░░░░░▄▀░░▄██▄░░▀▄░░░░░░░▄▀░" << std::endl;
    std::cout << "\t\t\t░▄▀░░░▄░▀▀░░░░████░░░░▀▀░▄░░░▀▄░" << std::endl;
    std::cout << "\t\t\t░█░░▄▄█▀▄▄░░░░░░░░░░░░░▄▄██▄░░█░" << std::endl;
    std::cout << "\t\t\t░▀▄░░░▀▄█░▀▀█▀▀█▀▀▀█▀▀█░█▀░░░▄▀░" << std::endl;
    std::cout << "\t\t\t░░▀▄░░░░▀▀▄▄█░░█░░░█░▄█▀░░░░▄▀░░" << std::endl;
    std::cout << "\t\t\t░░░░▀▄▄░░░░░▀▀▀▀▀▀▀▀▀░░░░▄▄▀░░░░" << std::endl;
    std::cout << "\t\t\t░░░░░░░▀▀▀▄▄▄▄▄▄▄▄▄▄▄▄▀▀▀░░░░░░░" << std::endl;
}
