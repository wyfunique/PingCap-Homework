#include "../include/memory.h"

namespace PingCap
{
	Memory::Memory(){}
	
	size_t Memory::getMemSize()
	{
		return size;
	}

	size_t Memory::getVirtualMemUsed()
	{
		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
		size_t virtual_memory_used = pmc.PrivateUsage;
		return virtual_memory_used;
	}

	size_t Memory::getPhysicalMemUsed()
	{
		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
		size_t physical_memory_used = pmc.WorkingSetSize;
		return physical_memory_used;
	}

	bool Memory::noFreeMem()
	{
		return getMemSize() - getPhysicalMemUsed() < NO_FREE_MEM_THRESH;
	}
}

