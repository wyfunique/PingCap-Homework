#include <iostream>
#include <time.h>
#include "include/memory.h"

using namespace PingCap;

void testAllGetMemRelated()
{
	Memory mem = Memory();
	std::cout << "Current total mem: " << mem.getMemSize(Unit::MB) << std::endl;
	std::cout << "Current virtual mem used: " << mem.getVirtualMemUsed(Unit::MB) << std::endl;
	std::cout << "Current physical mem used: " << mem.getPhysicalMemUsed(Unit::MB) << std::endl;
	std::cout << "Has free mem? " << mem.hasFreeMem() << std::endl;
}

void testLoadNextURL(Alg alg)
{
	Memory mem;
	std::fstream* f = mem.openFile("util/urls.txt", "r");
	mem.loadNextURL(f, alg);
	// Print one url and its count, 1, here, if correct
	mem.printCounter();
}

void testSaveOldURL(Alg alg)
{
	Memory mem;
	std::fstream* f = mem.openFile("util/urls.txt", "r");
	mem.loadNextURL(f, alg);
	std::cout << "Before saving: " << std::endl;
	// Print one url and its count, 1, here, if correct
	mem.printCounter();
	std::cout << "After saving: " << std::endl;
	mem.saveOldURL(alg);
	// Print nothing here, if correct
	mem.printCounter();
	// There will be a new file in folder "temp" named by this url, if correct
}

void testScanAllURLs(Alg alg)
{
	Memory mem;
	std::fstream* f = mem.openFile("util/urls.txt", "r");
	// Iterate and count on all urls
	uint64_t idx = 1;
	while (mem.loadNextURL(f, alg))
	{
		if (idx % 2000 == 0) 
		{
			std::cout << "No." << idx << " URL loaded. " << "Saving URL number: " << mem.getNumSaveURL() << std::endl;
			std::cout << "Memory used: " << mem.getVirtualMemUsed(Unit::MB) << "/" << mem.getMemSize(Unit::MB) << std::endl;
		}
		idx++;
	}
	// Finally there would be 1000 tmp files in folder 'temp/', as we set the number of distinct urls to 1000. 
}

float getRunningTime(void(*function)(Alg), Alg alg)
{
	clock_t start = clock();
	function(alg);
	clock_t end = clock();
	float diff = (float)end - (float)start;
	float seconds = diff / CLOCKS_PER_SEC;
	return seconds;
	//std::cout << "Running time: " << seconds << "s" << std::endl;
}

int main()
{
	//testLoadNextURL();
	//testSaveOldURL();
	//testScanAllURLs();
	float seconds = getRunningTime(&testScanAllURLs, Alg::MRU);
	std::cout << "Running time: " << seconds << "s" << std::endl;
	int tmp;
	std::cin >> tmp;
	return 0;
}