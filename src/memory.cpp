#include "../include/memory.h"

namespace PingCap
{
	Memory::Memory(){}
	
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
}

