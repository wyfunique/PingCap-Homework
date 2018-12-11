#include "../include/memory.h"

namespace PingCap
{
	Memory::Memory(std::string log_path)
	{
		counter = std::unordered_map<std::string, uint64_t>();
		LRU_queue = std::list<std::string>();
		logger = Logger(log_path);
	}
	
	float Memory::convertByUnit(uint64_t size_in_byte, Unit unit)
	{
		return size_in_byte / (float)unit;
	}

	float Memory::getMemSize(Unit unit)
	{
		return convertByUnit(size, unit);
	}

	float Memory::getVirtualMemUsed(Unit unit)
	{
		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
		uint64_t virtual_memory_used = pmc.PrivateUsage;
		return convertByUnit(virtual_memory_used, unit);
	}

	float Memory::getPhysicalMemUsed(Unit unit)
	{
		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
		uint64_t physical_memory_used = pmc.WorkingSetSize;
		return convertByUnit(physical_memory_used, unit);
	}

	bool Memory::hasFreeMem()
	{
		return getMemSize(Unit::BYTE) - getPhysicalMemUsed(Unit::BYTE) >= NO_FREE_MEM_THRESH;
	}

	void Memory::openFile(std::string filename, std::string mode)
	{
		if (mode == "r")
		{
			if (isInputFileOpened())
			{
				logger.printError("openFile", "Cannot open another file for reading because input stream is being occupied");
			}
			else
			{
				input_stream.open(filename, std::ifstream::in);
			}
		}
		else if (mode == "w")
		{
			if (isOutputFileOpened())
			{
				logger.printError("openFile", "Cannot open another file for writting because output stream is being occupied");
			}
			else
			{
				output_stream.open(filename, std::ifstream::out);
			}
		}
		else
		{
			logger.printError("openFile", "Unsupported mode when openning the file '" + filename + "'");
		}
	}
	/*
	void Memory::openInputFile(std::string filename)
	{
		input_stream.open(filename, std::ifstream::in);
	}

	void Memory::openOutputFile(std::string filename)
	{
		output_stream.open(filename, std::ifstream::out);
	}
	*/
	void Memory::closeInputFile()
	{
		input_stream.close();
	}

	void Memory::closeOutputFile()
	{
		output_stream.close();
	}

	bool Memory::isInputFileOpened()
	{
		return input_stream.is_open();
	}

	bool Memory::isOutputFileOpened()
	{
		return output_stream.is_open();
	}

	void Memory::loadNextURL()
	{
		std::string url;
		if (!hasFreeMem())
		{
			logger.printInfo("loadNextURL", "No free memory, so remove an old URL and write it to disk first.");
			saveURL(); // Save an old URL to disk and remove it from memory
		}
		if (std::getline(input_stream, url)) // Get next line/url successfully
		{
			// not present in memory 
			if (counter.find(url) == counter.end())
			{
				counter[url] = 1;
			}
			// present in memory 
			else
			{
				LRU_queue.erase();
			}
			// update reference 
			dq.push_front(x);
			ma[x] = dq.begin();
		}
	}

	// memory is full 
	if (dq.size() == csize)
	{
		//delete least recently used element 
		int last = dq.back();
		dq.pop_back();
		ma.erase(last);
	}

}

