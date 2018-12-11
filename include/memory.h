#ifndef MEMORY_H
#define MEMORY_H

#include <windows.h>
#include <psapi.h>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map> 
#include <string>
#include <unordered_map>
#include "../include/logger.h"

namespace PingCap 
{
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
	
	const Unit DEFAULT_UNIT = Unit::BYTE;
	//const uint64_t BYTE = 1;
	//const uint64_t KB = 1000 * BYTE;
	//const uint64_t MB = 1000 * KB;
	//const uint64_t GB = 1000 * MB;
	const uint64_t MEMORY_SIZE = 1 * GB;
	const uint64_t FILE_SIZE = 100 * GB;

	/*
		This threashold equals to (2 * maximum URL length + 8) bytes
		Since each time we load next URL into memory from disk, we need 
		(1) Space for one key (URL) and one value (frequency of the URL, type uint64_t, 8 bytes) in the map 'counter', that is, at most (maximum URL length + 8) bytes.
		(2) Space for one element (URL) in the list 'urls', that is, at most (maximum URL length) bytes.
	*/
	const uint64_t NO_FREE_MEM_THRESH = 308 * BYTE; 
	
	/*
		This class is abstract of main memory. 
		It provides information of memory status gotten through Windows API, and loads or saves items between memory and disk.
	*/
	class Memory
	{
		private:
			const uint64_t size = MEMORY_SIZE;
			PROCESS_MEMORY_COUNTERS_EX pmc;
			//size_t virtual_memory_used;
			std::unordered_map<std::string, uint64_t> counter;
			std::list<std::string> LRU_queue;
			std::ifstream input_stream;
			std::ofstream output_stream;
			Logger logger;

			float convertByUnit(uint64_t size_in_byte, Unit unit = DEFAULT_UNIT);

		public:

			Memory(std::string log_path);

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
			void openFile(std::string filename, std::string mode);
			/*
				Opens a file for reading from
			*/
			//void openInputFile(std::string filename);

			/*
				Opens a file for writting to 
			*/
			//void openOutputFile(std::string filename);
			
			/*
			Closes the opened input file
			*/
			void closeInputFile();

			/*
			Closes the opened output file
			*/
			void closeOutputFile();

			/*
				Checks if the ifstrean 'in' is assigned to a file
			*/
			bool isInputFileOpened();

			/*
				Checks if the ofstrean 'out' is assigned to a file
			*/
			bool isOutputFileOpened();

			/*
				Loads next url from the file into counter using LRU algorithm.
			*/
			void loadNextURL();

			
		/*
			This function opens and loads content from the file with given filename	
		void load(std::string filename);
		*/
		
	};
}



#endif
