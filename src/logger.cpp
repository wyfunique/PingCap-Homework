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
	/*
	std::string Logger::getCurTime()
	{
		time_t rawtime;
		struct tm timeinfo;
		time(&rawtime);
		localtime_s(&timeinfo, &rawtime);
		return asctime_s(&timeinfo);
		//printf("Current local time and date: %s", asctime(timeinfo));
	}
	*/
	void Logger::logError(std::string func_name, std::string error)
	{
		//time_t now = time(0);
		//char* datetime = ctime(&now);
		//std::string datetime(ctime(&now));
		//log << "[" + getCurTime() + "] Error: " + error << std::endl;
		log << "Error (in " + func_name + "): " + error << std::endl;
	}
	void Logger::logInfo(std::string func_name, std::string info)
	{
		//time_t now = time(0);
		//char* datetime = ctime(&now);
		//std::string datetime(ctime(&now));
		//log << "[" + getCurTime() + "] Info: " + info << std::endl;
		log << "Info (in " + func_name + "): " + info << std::endl;
	}
	void Logger::printError(std::string func_name, std::string error)
	{
		//time_t now = time(0);
		//char* datetime = ctime(&now);
		//std::string datetime(ctime(&now));
		//std::cout << "[" + getCurTime() + "] Error: " + error << std::endl;
		std::cout << "Error (in " + func_name + "): " + error << std::endl;
	}
	void Logger::printInfo(std::string func_name, std::string info)
	{
		//time_t now = time(0);
		//char* datetime = ctime(&now);
		//std::string datetime(ctime(&now));
		//std::cout << "[" + getCurTime() + "] Info: " + info << std::endl;
		std::cout << "Info (in " + func_name + "): " + info << std::endl;
	}
	

}