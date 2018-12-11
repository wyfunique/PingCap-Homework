#ifndef LOGGER_H
#define LOGGER_H

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

namespace PingCap
{
	/*
		This class is the controller for logging.
	*/
	class Logger
	{
		private:
			std::ofstream log;
			std::string getCurTime();

		public:
			Logger();
			Logger(std::string log_path);
			void logError(std::string func_name, std::string error);
			void logInfo(std::string func_name, std::string info);
			void printError(std::string func_name, std::string error);
			void printInfo(std::string func_name, std::string info);
			
	};
}

#endif