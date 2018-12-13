#ifndef MEMORY_H
#define MEMORY_H

#include <algorithm>
#include <direct.h>
#include <windows.h>
#include <psapi.h>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <map> 
#include <list>
#include <string>
#include <unordered_map>
#include <filesystem>
#include "../include/logger.h"


namespace PingCap 
{
	/*
		We implemented two URL replacement algorithms.
	*/
	enum Alg:int
	{
		LRU = 0,
		MRU = 1
	};

	/*
		In case of integer overflow, we use uint64_t for memory and file sizes.
	*/
	enum Unit:uint64_t
	{
		BYTE = 1,
		KB = 1000 * BYTE,
		MB = 1000 * KB,
		GB = 1000 * MB
	};
	
	// TODO: Write all config parameters to config file and read the file each time when starting.
	const Unit DEFAULT_UNIT = Unit::BYTE;
	const uint64_t MEMORY_SIZE = 5 * MB;
	
	/*
		This threashold equals to (2 * maximum URL length + 8) bytes
		Since each time we load next URL into memory from disk, we need 
		(1) Space for one key (URL) and one value (frequency of the URL, type uint64_t, 8 bytes) in the map 'counter', that is, at most (maximum URL length + 8) bytes.
		(2) Space for one element (URL) in the list 'urls', that is, at most (maximum URL length) bytes.
	*/
	const uint64_t NO_FREE_MEM_THRESH = 308 * BYTE; 
	
	const std::string TEMP_FILE_DIR = "temp\\";
	const std::string LOG_FILE = "log.txt";
	const std::string URL_FILE = "util\\urls.txt";
	const std::string PROC_URL_FILE = "util\\proc_urls_1.txt";

	/*
		This class is abstract of main memory. 
		It provides information of memory status gotten through Windows API, and loads or saves items between memory and disk.
	*/
	class Memory
	{
		private:
			const uint64_t size = MEMORY_SIZE;
			const std::string temp_file_dir = TEMP_FILE_DIR;
			const std::string log_file = LOG_FILE;
			const std::string url_file = URL_FILE;
			const std::string proc_url_file = PROC_URL_FILE;

			PROCESS_MEMORY_COUNTERS_EX pmc;
			std::unordered_map<std::string, std::pair<uint64_t, std::list<std::string>::iterator>> counter;
			std::list<std::string> LRU_queue; // list needs O(1) time to push front, while vector needs O(n) time. So we use list for LRU.
			std::unordered_map<std::string, std::fstream*> opened_files; // map and unordered map will copy the reference, so here we use pointers of the file streams to avoid copying.
			Logger logger;
			// The number of saving URL to disk
			uint64_t num_save_URL;

			/*
				Convert value to given unit
			*/
			float convertByUnit(uint64_t size_in_byte, Unit unit = DEFAULT_UNIT);

			/*
				Encode URL to be valid filename
			*/
			std::string encodeURLAsFilename(std::string url);
			
			/*
				Check if a file is empty
			*/
			bool isFileEmpty(std::fstream* f);

		public:

			Memory();

			/*
				Print stuff in map 'counter'
			*/
			void printCounter();

			/*
				Get total memory size displayed in given unit
			*/
			float getMemSize(Unit unit = DEFAULT_UNIT);
			
			/*
				Get used virtual memory size displayed in given unit
			*/
			float getVirtualMemUsed(Unit unit = DEFAULT_UNIT);

			/*
				Get used physical memory size displayed in given unit
			*/
			float getPhysicalMemUsed(Unit unit = DEFAULT_UNIT);

			/*
				Returns if there is free memory. When free memory is less than NO_FREE_MEM_THRESH, consider it as "No free memory".
 			*/
			bool hasFreeMem();
			
			/*
				Opens a file for reading from or writting to 
				@ mode: "r" means reading while "w" means writting
			*/
			std::fstream* openFile(std::string filename, std::string mode);

			/*
				Close the opened file with given filename
			*/
			void closeFile(std::string filename);

			/*
				Check if the given file is opened
			*/
			bool isFileOpened(std::string filename);

			/*
				Preprocess URL list file
				@src_path: the original URL list file path
				@dst_path: the path of file after processing
			*/
			void procURLs(std::string src_path, std::string dst_path);

			/*
				Loads next url from the file into counter using given replacement algorithm.
			*/
			bool loadNextURL(std::fstream* input_stream, Alg alg);

			/*
				Loads next url from the pre-processed file into counter using given replacement algorithm.
			*/
			bool loadNextURLAfterProc(std::fstream* proc_input_stream, Alg alg);

			/*
				Save and remove an old URL from memory using given algorithm 
			*/
			void saveOldURL(Alg alg);
			
			/*
				Get the number of saving urls until now
			*/
			uint64_t getNumSaveURL();

			/*
				List all filenames under given dir
			*/
			std::vector<std::string> getFileNamesInDirectory(std::string directory);

			/*
				Get the top K frequent items from URL list file
			*/
			std::vector<std::pair<uint64_t, std::string>> getTopKFreqItems(int k, Alg alg);

	};
}



#endif
