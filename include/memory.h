#ifndef MEMORY_H
#define MEMORY_H

#include <windows.h>
#include <psapi.h>
#include <cstdint>
#include <iostream>
#include <string>

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
	
	const Unit DEFAULT_UNIT = Unit::MB;
	//const uint64_t BYTE = 1;
	//const uint64_t KB = 1000 * BYTE;
	//const uint64_t MB = 1000 * KB;
	//const uint64_t GB = 1000 * MB;
	const uint64_t MEMORY_SIZE = 1 * GB;
	const uint64_t FILE_SIZE = 100 * GB;
	const uint64_t NO_FREE_MEM_THRESH = 50 * BYTE;
	
	/*
		This class is abstract of main memory. 
		It provides information of memory status gotten through Windows API. 
	*/
	class Memory
	{
		private:
			const uint64_t size = MEMORY_SIZE;
			PROCESS_MEMORY_COUNTERS_EX pmc;
			//size_t virtual_memory_used;

			float convertByUnit(uint64_t size_in_byte, Unit unit = DEFAULT_UNIT);

		public:

			Memory();

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
			This function opens and loads content from the file with given filename	
		void load(std::string filename);
		*/
		
	};
}



#endif
