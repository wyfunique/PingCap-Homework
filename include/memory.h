#ifndef MEMORY_H
#define MEMORY_H

#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <string>

namespace PingCap 
{
	const size_t BYTE = 1;
	const size_t KB = 1000 * BYTE;
	const size_t MB = 1000 * KB;
	const size_t GB = 1000 * MB;
	const size_t MEMORY_SIZE = 1 * GB;
	const size_t FILE_SIZE = 100 * GB;
	const size_t NO_FREE_MEM_THRESH = 50 * BYTE;
	/*
		This class is abstract of main memory. 
		It provides information of memory status gotten through Windows API. 
	*/
	class Memory
	{
		private:
			const size_t size = MEMORY_SIZE;
			PROCESS_MEMORY_COUNTERS_EX pmc;
			//size_t virtual_memory_used;

		public:

			Memory();
			size_t getMemSize();
			size_t getVirtualMemUsed();
			size_t getPhysicalMemUsed();
			bool noFreeMem();
		
		/*
			This function opens and loads content from the file with given filename	
		void load(std::string filename);
		*/
		
	};
}



#endif#pragma once
