#include <iostream>
#include <fstream>
#include "Config.h"
#include "Logger.h"
#include "CurrentTime.h"

using namespace std;

Logger* Logger::mThis = NULL;

ostream& operator << (ostream& os, LogLevel level)
{
    switch(level){
    case LL_ERROR:
        os << "ERROR";
        break;
    case LL_WARNING:
        os << "WARNING";
        break;
    case LL_INFO:
        os << "INFO";
        break;
    case LL_DEBUG:
        os << "DEBUG";
        break;
    case LL_TRACE:
        os << "TRACE";
        break;
    default:
        os << "UNKNOWN";
        break;
    }
    return os;
}

Logger::Logger(const char* logFileName)
{
    mThis = this;

    mLogFile = new ofstream(logFileName);
    if (! *mLogFile){
        cout << "Could not open log file " << logFileName << endl;
        exit(-1);
    }

    mMaxLevel = (LogLevel)confParam<size_t>("system.log_level");
}


Logger::~Logger()
{
    *mLogFile << endl;
    mLogFile->flush();
    mLogFile->close();
}

bool
Logger::logable(LogLevel level, const char* file, const char* function)
{
    return level <= mMaxLevel;
}
 
void 
Logger::logInfo(LogLevel level, const char* file, 
                const char* function)
{
    string sFile(file);
    string fileWithoutPath = sFile.substr(sFile.find('/') + 1);
    *mLogFile << endl << currentDateTime() << " "
              << level << " " << fileWithoutPath << ":" 
              << function << "() --- ";

    // LL_ERROR messages are also written to cout
    if (level == LL_ERROR || level == LL_WARNING)
        cerr << currentDateTime() << " "
             << level << " " << fileWithoutPath << ":" 
             << function << "()" << endl;
}

void 
Logger::logMessage(const std::string& entry)
{
    *mLogFile << entry;
    mLogFile->flush();
}

