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
#include <thread>
#include <mutex>
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
	const uint64_t MEMORY_SIZE = 7 * MB;
	
	/*
		This threashold equals to (2 * maximum URL length + 8) bytes
		Since each time we load next URL into memory from disk, we need 
		(1) Space for one key (URL) and one value (frequency of the URL, type uint64_t, 8 bytes) in the map 'counter', that is, at most (maximum URL length + 8) bytes.
		(2) Space for one element (URL) in the list 'urls', that is, at most (maximum URL length) bytes.
	*/
	const uint64_t MAX_URL_LENGTH = 150 * BYTE;
	const uint64_t NO_FREE_MEM_THRESH = 2 * MAX_URL_LENGTH + 8;
	
	const std::string TEMP_FILE_DIR = "temp\\";
	const std::string LOG_FILE = "log.txt";
	const std::string URL_FILE = "util\\urls.txt";
	const std::string PROC_URL_FILE = "util\\proc_urls_1.txt";
	const float TEMP_FILE_SIZE_RATIO = 0.01;
	const uint64_t TEMP_FILE_SIZE = TEMP_FILE_SIZE_RATIO * MEMORY_SIZE;
	const uint64_t TEMP_FILE_AMOUNT = 100 / TEMP_FILE_SIZE_RATIO;

	const float READ_BUFFER_RATIO = 0.2;
	const float WRITE_BUFFER_RATIO = 0.3;
	const uint64_t READ_BUFFER_SIZE = MEMORY_SIZE * READ_BUFFER_RATIO;
	const uint64_t WRITE_BUFFER_SIZE = MEMORY_SIZE * WRITE_BUFFER_RATIO;
	const uint64_t NO_FREE_BUFFER_THRESH = MAX_URL_LENGTH;

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

			std::vector<std::string> read_buffer;
			std::vector<std::pair<std::string, uint64_t>> write_buffer;
			const uint64_t max_read_buffer_size = READ_BUFFER_SIZE;
			const uint64_t max_write_buffer_size = WRITE_BUFFER_SIZE;

			Memory();

			Memory(int num_writer_threads);

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

			/*
				Get size of read buffer
			*/
			//uint64_t getReadBufferSize();

			/*
				Get size of write buffer
			*/
			//uint64_t getWriteBufferSize();

			/*
			Check if the read buffer is full
			*/
			bool isReadBufferFull();

			/*
			Check if the read buffer is empty
			*/
			bool isReadBufferEmpty();

			/*
			Check if the write buffer is full
			*/
			bool isWriteBufferFull();

			/*
			Check if the write buffer is empty
			*/
			bool isWriteBufferEmpty();

	};

	/*
		This class implements thread of writer to write records from memory to disk.
	*/
	class Writer
	{
		private:
			std::string name;
			uint64_t max_secd_buf_size; 
			uint64_t temp_file_amount;
			uint64_t no_free_buffer_thresh;
			// secd_write_buffer is the local write buffer of every writer thread
			std::map<std::string, uint64_t> secd_write_buffer;
			Memory* memory;
		
		public:
			
			Writer();
			
			Writer(Memory* mem, uint64_t write_buffer_size, uint64_t temp_file_amount, uint64_t global_no_free_buffer_thresh, std::string name);
			
			/*
				Get the index of hash bucket of given URL
			*/
			size_t getTempFileIndex(std::string url);

			/*
				Check if local write buffer is full
			*/
			bool isSecdBufFull();

			/*
				Check if local write buffer is empty
			*/
			bool isSecdBuffEmpty();

			/*
				Main function for reading records from main write buffer and saving records to disk
			*/
			static void write(Writer* writer);

			/*
				Run the writer thread
			*/
			void start();
	};
	
}

#endif
