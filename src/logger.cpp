#include "../include/logger.h"

namespace PingCap
{
	Logger::Logger() {}

	Logger::Logger(std::string log_path)
	{
		log.open(log_path, std::ofstream::out | std::ofstream::trunc);
		log.close();
		log.open(log_path, std::ofstream::out | std::ofstream::app);
	}
	
	void Logger::logError(std::string func_name, std::string error)
	{
		log << "Error (in " + func_name + "): " + error << std::endl;
	}
	void Logger::logInfo(std::string func_name, std::string info)
	{
		log << "Info (in " + func_name + "): " + info << std::endl;
	}
	void Logger::printError(std::string func_name, std::string error)
	{
		std::cout << "Error (in " + func_name + "): " + error << std::endl;
	}
	void Logger::printInfo(std::string func_name, std::string info)
	{
		std::cout << "Info (in " + func_name + "): " + info << std::endl;
	}
}