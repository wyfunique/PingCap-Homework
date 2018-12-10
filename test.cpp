#include <iostream>
#include "include/memory.h"

using namespace PingCap;

int main()
{
	Memory mem = Memory();
	std::cout << "Current total mem: " << mem.getMemSize(Unit::MB) << std::endl;
	std::cout << "Current virtual mem used: " << mem.getVirtualMemUsed(Unit::MB) << std::endl;
	std::cout << "Current physical mem used: " << mem.getPhysicalMemUsed(Unit::MB) << std::endl;
	std::cout << "Has free mem? " << mem.hasFreeMem() << std::endl;
	int tmp;
	std::cin >> tmp;
	return 0;
}