#include "../include/memory.h"

namespace PingCap
{
	Memory::Memory()
	{
		counter = std::unordered_map<std::string, std::pair<uint64_t, std::list<std::string>::iterator>>();
		LRU_queue = std::list<std::string>();
		opened_files = std::unordered_map<std::string, std::fstream*>();
		logger = Logger(log_file);
		//logger.logInfo("Memory", "HAHA");
		
		//std::filesystem
		//logger.logInfo("Memory", std::to_string(PathFileExistsA(temp_file_dir.c_str())));
		//char buf[256];
		//GetCurrentDirectoryA(256, buf);
		//std::string tmp = std::string(buf) + std::string("\\") + temp_file_dir;
		//logger.logInfo("Memory", tmp);

		// Check if temp_file_dir exists and create it if absent.
		DWORD ftyp = GetFileAttributesA(temp_file_dir.c_str());
		//std::cout << GetLastError() << std::endl;
		// Something is wrong with the path
		if (ftyp == INVALID_FILE_ATTRIBUTES)
		{
			if (GetLastError() == ERROR_FILE_NOT_FOUND)
			{
				_mkdir(temp_file_dir.c_str());
			}
			else
			{
				logger.logError("Memory", "Invaid temp_file_dir");
				exit(1);
			}
		}
	}
	
	float Memory::convertByUnit(uint64_t size_in_byte, Unit unit)
	{
		return size_in_byte / (float)unit;
	}

	std::string Memory::encodeURLAsFilename(std::string url)
	{
		std::string url_copy = url;
		std::replace(url_copy.begin(), url_copy.end(), '/', '%');
		std::replace(url_copy.begin(), url_copy.end(), ':', '%');
		return url_copy;
	}

	bool Memory::isFileEmpty(std::fstream* f)
	{
		return f->peek() == std::fstream::traits_type::eof();
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

	std::fstream* Memory::openFile(std::string filename, std::string mode)
	{
		if (isFileOpened(filename))
		{
			logger.printInfo("openFile", "File '" + filename + "' has been opened.");
			return opened_files[filename];
		}
		if (mode == "r")
		{
			std::fstream* input_stream = new std::fstream(filename, std::fstream::in);
			//input_stream.open(filename, std::fstream::in);
			opened_files[filename] = input_stream;	
			return input_stream;
		}
		else if (mode == "w")
		{
			std::fstream* output_stream = new std::fstream(filename, std::fstream::out | std::fstream::trunc);
			//output_stream.open(filename, std::fstream::out | std::fstream::trunc);
			opened_files[filename] = output_stream;
			return output_stream;
		}
		else if (mode == "a")
		{
			std::fstream* output_stream = new std::fstream(filename, std::fstream::out | std::fstream::app);
			//output_stream.open(filename, std::fstream::out | std::fstream::app);
			opened_files[filename] = output_stream;
			return output_stream;
		}
		else
		{
			logger.printError("openFile", "Unsupported mode when openning the file '" + filename + "'");
			return NULL;
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
	
	void Memory::closeFile(std::string filename)
	{
		std::unordered_map<std::string, std::fstream*>::iterator iter = opened_files.find(filename);
		// The file is not opened
		if (iter == opened_files.end())
		{
			logger.printError("closeFile", "File '" + filename + "' is not opened.");
		}
		// The file is opened
		else
		{
			std::fstream* f = opened_files[filename];
			f->close();
			opened_files.erase(iter);
		}

	}

	/*
	void Memory::closeInputFile()
	{
		input_stream.close();
	}

	void Memory::closeOutputFile()
	{
		output_stream.close();
	}
	*/

	bool Memory::isFileOpened(std::string filename)
	{
		return opened_files.find(filename) != opened_files.end();
	}

	/*
	bool Memory::isInputFileOpened()
	{
		return input_stream.is_open();
	}

	bool Memory::isOutputFileOpened()
	{
		return output_stream.is_open();
	}
	*/

	void Memory::loadNextURL(std::fstream input_stream)
	{
		std::string url;
		if (!hasFreeMem())
		{
			logger.printInfo("loadNextURL", "No free memory, so remove an old URL and write it to disk first.");
			saveOldURL(); // Save the least recently used URL to disk and remove it from memory
		}
		if (std::getline(input_stream, url)) // Get next line/url successfully
		{
			//std::unordered_map<std::string, std::pair<uint64_t, std::vector<std::string>::iterator>>::iterator idx = counter.find(url);
			// not present in memory 
			if (counter.find(url) == counter.end())
			{
				LRU_queue.push_front(url);
				counter[url] = std::pair<uint64_t, std::list<std::string>::iterator>(1, LRU_queue.begin());
			}
			// present in memory 
			else
			{
				LRU_queue.erase(counter[url].second);
				LRU_queue.push_front(url);
				counter[url] = std::pair<uint64_t, std::list<std::string>::iterator>(1 + counter[url].first, LRU_queue.begin());
			}
		}
	}

	void Memory::saveOldURL()
	{
		// Get the least recently used URL and remove it from LRU queue
		std::string last_url = LRU_queue.back();
		uint64_t count = counter[last_url].first;
		LRU_queue.pop_back();
		
		// Save the URL into its temp file
		// First, read the temp file to check if it has content, that is, record when we save URL last time 
		std::string last_url_encoded = encodeURLAsFilename(last_url);
		std::string temp_file_path = temp_file_dir + last_url_encoded + ".tmp";
		std::fstream* input_stream = openFile(temp_file_path, "r");
		uint64_t last_count = 0;

		// If the file is not empty, then this is not the first time we save this URL.
		if (!isFileEmpty(input_stream))
		{
			// Read last count of this URL
			*input_stream >> last_url >> last_count;
			/*
			closeFile(temp_file_path);
			std::fstream* output_stream = openFile(temp_file_path, "a");
			(*output_stream) << last_url << " " << count;
			//output_stream << counter[last_url].first;
			closeFile(temp_file_path);
			*/
		}
		closeFile(temp_file_path);
		std::fstream* output_stream = openFile(temp_file_path, "w");
		
		// Calculate and save the total count until now
		(*output_stream) << last_url << " " << (last_count + count); 
		closeFile(temp_file_path);
		
		// Remove the URL from map 'counter'
		counter.erase(last_url);
		logger.logInfo("saveOldURL", "Save URL '" + last_url + "' (count: " + std::to_string(count) + ") into temp file");
	}
}

