#include "../include/memory.h"

namespace PingCap
{
	//std::mutex* read_buffer_lock = new std::mutex();
	
	// Lock for main write buffer
	std::mutex* write_buffer_lock = new std::mutex();
	// Locks for every temp file
	std::vector<std::mutex*> file_locks;

	Memory::Memory() {}

	Memory::Memory(int num_writer_threads)
	{
		counter = std::unordered_map<std::string, std::pair<uint64_t, std::list<std::string>::iterator>>();
		LRU_queue = std::list<std::string>();
		opened_files = std::unordered_map<std::string, std::fstream*>();
		logger = Logger(log_file);
		num_save_URL = 0;
		// Check if temp_file_dir exists and create it if absent.
		DWORD ftyp = GetFileAttributesA(temp_file_dir.c_str());
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

		// Initialize Locks and writer threads
		for (int i = 0; i < TEMP_FILE_AMOUNT; i++)
		{
			file_locks.push_back(new std::mutex());
		}
		for (int i = 0; i < num_writer_threads; i++)
		{
			(new Writer(this, WRITE_BUFFER_SIZE, TEMP_FILE_AMOUNT, NO_FREE_BUFFER_THRESH, "writer-"+std::to_string(i)))->start();
			logger.logInfo("Memory", "Writer thread " + std::to_string(i) + " start");
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

	void Memory::printCounter()
	{
		for (auto const& pair : counter)
		{
			std::string url = pair.first;
			uint64_t count = pair.second.first;
			std::cout << count << " " << url << std::endl;
		}
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
		// Convert all three numbers to int64_t in order to avoid underflow if getMemSize < getPhysicalMemUsed
		// NOTE: When comparing signed with unsigned, the compiler converts the signed value to unsigned.
		return (int64_t)getMemSize(Unit::BYTE) - (int64_t)getVirtualMemUsed(Unit::BYTE) >= (int64_t)NO_FREE_MEM_THRESH;
	}

	std::fstream* Memory::openFile(std::string filename, std::string mode)
	{
		if (isFileOpened(filename))
		{
			logger.logInfo("openFile", "File '" + filename + "' has been opened.");
			return opened_files[filename];
		}
		if (mode == "r")
		{
			std::fstream* input_stream = new std::fstream(filename, std::fstream::in);
			opened_files[filename] = input_stream;	
			return input_stream;
		}
		else if (mode == "w")
		{
			std::fstream* output_stream = new std::fstream(filename, std::fstream::out | std::fstream::trunc);
			opened_files[filename] = output_stream;
			return output_stream;
		}
		else if (mode == "a")
		{
			std::fstream* output_stream = new std::fstream(filename, std::fstream::out | std::fstream::app);
			opened_files[filename] = output_stream;
			return output_stream;
		}
		else
		{
			logger.logError("openFile", "Unsupported mode when openning the file '" + filename + "'");
			return NULL;
		}
	}
	
	void Memory::closeFile(std::string filename)
	{
		std::unordered_map<std::string, std::fstream*>::iterator iter = opened_files.find(filename);
		// The file is not opened
		if (iter == opened_files.end())
		{
			logger.logError("closeFile", "File '" + filename + "' is not opened.");
		}
		// The file is opened
		else
		{
			std::fstream* f = opened_files[filename];
			f->close();
			// Remember delete f here since f is created by new, and only close() cannot release memory occupied by the fstream itself.
			delete f; 
			opened_files.erase(iter);
		}

	}

	bool Memory::isFileOpened(std::string filename)
	{
		return opened_files.find(filename) != opened_files.end();
	}

	void Memory::procURLs(std::string src_path, std::string dst_path)
	{
		std::cout << "Pre-processing URL list file..." << std::endl;

		std::ifstream input_stream(src_path, std::ifstream::in);

		std::ofstream output_stream(dst_path, std::ofstream::out | std::ofstream::trunc);
		output_stream.close();
		output_stream.open(dst_path, std::ofstream::out | std::ofstream::app);

		std::unordered_map<std::string, uint64_t> tmp_counter;
		uint64_t idx = 0;
		std::string line;
		std::string url;
		std::string count_str;
		uint64_t count;
		while (input_stream >> line)
		{
			idx++;

			int space = line.find(" ");
			if (space == std::string::npos)
			{
				url = line;
				count = 1;
			}
			else
			{
				url = line.substr(0, space);
				count = std::stoull(line.substr(space+1));
			}
			if (!hasFreeMem())
			{
				std::cout << "Processed " << idx << " urls" << std::endl;
				for (auto const& pair : tmp_counter)
				{
					output_stream << pair.first << " " << pair.second << std::endl;
				}
				tmp_counter.clear();
			}
			if (tmp_counter.find(url) == tmp_counter.end())
			{
				tmp_counter[url] = count;
			}
			else
			{
				tmp_counter[url] += count;
			}
			
		}
		
		std::cout << "Done" << std::endl;
		if (!tmp_counter.empty())
		{
			for (auto const& pair : tmp_counter)
			{
				output_stream << pair.first << " " << pair.second << std::endl;
			}
		}
		input_stream.close();
		output_stream.close();
	}

	bool Memory::loadNextURL(std::fstream* input_stream, Alg alg)
	{
		std::string url;
		if (!hasFreeMem())
		{
			saveOldURL(alg); // Save the least recently used URL to disk and remove it from memory
		}
		if (std::getline(*input_stream, url)) // Get next line/url successfully
		{
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
			// Load successfully
			return true; 
		}
		// No more urls to load
		return false;
	}

	bool Memory::loadNextURLAfterProc(std::fstream* proc_input_stream, Alg alg)
	{
		std::string url;
		uint64_t count;
		
		if (!hasFreeMem())
		{
			saveOldURL(alg); // Save the least recently used URL to disk and remove it from memory
		}
		if (*proc_input_stream >> url >> count) // Get next line/url successfully
		{
			// not present in memory 
			if (counter.find(url) == counter.end())
			{
				LRU_queue.push_front(url);
				counter[url] = std::pair<uint64_t, std::list<std::string>::iterator>(count, LRU_queue.begin());
			}
			// present in memory 
			else
			{
				LRU_queue.erase(counter[url].second);
				LRU_queue.push_front(url);
				counter[url] = std::pair<uint64_t, std::list<std::string>::iterator>(count + counter[url].first, LRU_queue.begin());
			}
			// Load successfully
			return true;
		}
		// No more urls to load
		return false;
	}
	/*
	void Memory::saveOldURL(Alg alg)
	{
		// Get the least recently used URL and remove it from LRU queue
		
		// Now you have no free memory and there is no record in your memory. 
		// This means your memory is not enough initially. The game cannot start.
		if (LRU_queue.size() == 0)
		{
			logger.logError("saveOldURL", "Your memory is too small! This program cannot run.");
			exit(1);
		}

		std::string replaced_url; 
		if (alg == Alg::LRU)
		{
			replaced_url = LRU_queue.back();
			LRU_queue.pop_back();
		}
		else if (alg == Alg::MRU)
		{
			replaced_url = LRU_queue.front();
			LRU_queue.pop_front();
		}
		else
		{
			logger.logError("saveOldURL", "Unsupported URL replacement algorithm");
			exit(1);
		}

		uint64_t count = counter[replaced_url].first;
		
		// Save the URL into its temp file
		// First, read the temp file to check if it has content, that is, record when we save URL last time 
		std::string replaced_url_encoded = encodeURLAsFilename(replaced_url);
		std::string temp_file_path = temp_file_dir + replaced_url_encoded + ".tmp";
		std::fstream* input_stream = openFile(temp_file_path, "r");
		uint64_t last_count = 0;

		// If the file is not empty, then this is not the first time we save this URL.
		if (!isFileEmpty(input_stream))
		{
			// Read last count of this URL
			*input_stream >> replaced_url >> last_count;
		}
		closeFile(temp_file_path);
		input_stream = NULL;
		std::fstream* output_stream = openFile(temp_file_path, "w");
		
		// Calculate and save the total count until now
		(*output_stream) << replaced_url << " " << (last_count + count);
		closeFile(temp_file_path);
		output_stream = NULL;

		// Remove the URL from map 'counter'
		counter.erase(replaced_url);

		num_save_URL++;
	}
	*/
	void Memory::saveOldURL(Alg alg)
	{
		// Get the least recently used URL and remove it from LRU queue

		// Now you have no free memory and there is no record in your memory. 
		// This means your memory is not enough initially. The game cannot start.
		if (LRU_queue.size() == 0)
		{
			logger.logError("saveOldURL", "Your memory is too small! This program cannot run.");
			exit(1);
		}

		std::string replaced_url;
		if (alg == Alg::LRU)
		{
			replaced_url = LRU_queue.back();
			LRU_queue.pop_back();
		}
		else if (alg == Alg::MRU)
		{
			replaced_url = LRU_queue.front();
			LRU_queue.pop_front();
		}
		else
		{
			logger.logError("saveOldURL", "Unsupported URL replacement algorithm");
			exit(1);
		}

		uint64_t count = counter[replaced_url].first;

		// Save the URL into main write buffer
		write_buffer_lock->lock();
		// If main write buffer is full, release lock and sleep for a while
		while (isWriteBufferFull()) 
		{
			logger.logInfo("saveOldURL", "Write buffer is full");
			write_buffer_lock->unlock();
			Sleep(3000);
			write_buffer_lock->lock();
		}
		
		write_buffer.push_back(std::pair<std::string, uint64_t>(replaced_url, count));
	
		// Remove the URL from map 'counter'
		counter.erase(replaced_url);

		num_save_URL++;
		write_buffer_lock->unlock();
	}

	uint64_t Memory::getNumSaveURL()
	{
		return num_save_URL;
	}

	std::vector<std::string> Memory::getFileNamesInDirectory(std::string directory) 
	{
		std::vector<std::string> files;
		WIN32_FIND_DATA fileData;
		HANDLE hFind;

		if (!((hFind = FindFirstFile(directory.c_str(), &fileData)) == INVALID_HANDLE_VALUE)) 
		{
			while (FindNextFile(hFind, &fileData)) 
			{
				files.push_back(fileData.cFileName);
			}
		}
		FindClose(hFind);
		return files;
	}


	std::vector<std::pair<uint64_t, std::string>> Memory::getTopKFreqItems(int k, Alg alg)
	{
		procURLs(url_file, proc_url_file);
		std::fstream* f = openFile(proc_url_file, "r");
		// Iterate and count on all urls
		uint64_t idx = 1;
		while (loadNextURLAfterProc(f, alg))
		{
			if (idx % 2000 == 0)
			{
				std::cout << "No." << idx << " URL loaded. " << "Saving URL number: " << getNumSaveURL() << std::endl;
				std::cout << "Memory used: " << getVirtualMemUsed(Unit::MB) << "/" << getMemSize(Unit::MB) << std::endl;
			}
			idx++;
		}

		std::vector<std::pair<uint64_t, std::string>> top_k;
		std::vector<std::string> files = getFileNamesInDirectory(temp_file_dir + "*");
		for (int i = 0; i < files.size(); i++)
		{
			if (files[i] == ".." || files[i] == ".")
			{
				continue;
			}
			std::string file_path = temp_file_dir + files[i];
			std::string url;
			uint64_t freq;
			std::ifstream f(file_path);
			f >> url >> freq;
			top_k.push_back(std::pair<uint64_t, std::string>(freq, url));
			if (top_k.size() > k)
			{
				std::sort(top_k.rbegin(), top_k.rend());
				top_k.pop_back();
			}
		}
		return top_k;
	}
	
	bool Memory::isReadBufferFull()
	{
		uint64_t cur_buffer_size = 0; // in bytes
		for (int i = 0; i < read_buffer.size(); i++)
		{
			cur_buffer_size += read_buffer[i].size();
		}
		return (max_read_buffer_size - cur_buffer_size < NO_FREE_BUFFER_THRESH);
	}

	bool Memory::isReadBufferEmpty()
	{
		return (read_buffer.size() == 0);
	}

	bool Memory::isWriteBufferFull()
	{
		uint64_t cur_buffer_size = 0; // in bytes
		for (int i = 0; i < write_buffer.size(); i++)
		{
			std::cout << i << std::endl;
			cur_buffer_size += write_buffer[i].first.size();
			std::cout << i << std::endl;
		}
		return ((int64_t)max_write_buffer_size - (int64_t)cur_buffer_size < (int64_t)NO_FREE_BUFFER_THRESH);
	}

	bool Memory::isWriteBufferEmpty()
	{
		return (write_buffer.size() == 0);
	}

	// =======================================================================================

	Writer::Writer() {}

	Writer::Writer(Memory* mem, uint64_t write_buffer_size, uint64_t global_temp_file_amount, uint64_t global_no_free_buffer_thresh, std::string thread_name)
	{
		memory = mem;
		max_secd_buf_size = write_buffer_size * 0.5;
		temp_file_amount = global_temp_file_amount;
		no_free_buffer_thresh = global_no_free_buffer_thresh;
		name = thread_name;
	}

	size_t Writer::getTempFileIndex(std::string url)
	{
		std::hash<std::string> str_hash;
		size_t hash_res = str_hash(url); 
		// Since hash result is 32 bit, every subset has a length of (2^32 / temp_file_amount)
		// Then we can get the index of temp file that the given URL should write to
		return hash_res / (size_t)(pow(2, 32) / temp_file_amount);
	}

	bool Writer::isSecdBufFull()
	{
		uint64_t cur_buffer_size = 0; // in bytes
		for (auto record : secd_write_buffer)
		{
			cur_buffer_size += record.first.size();
		}
		return (max_secd_buf_size - cur_buffer_size < no_free_buffer_thresh);
	}

	bool Writer::isSecdBuffEmpty()
	{
		return (secd_write_buffer.size() == 0);
	}

	void Writer::write(Writer* writer)
	{
		while (true)
		{
			// First get the lock for main write buffer
			write_buffer_lock->lock();

			// Main write buffer is not empty
			if (!writer->memory->isWriteBufferEmpty()) 
			{
				// Local write buffer is full, then start writing to disk
				if (writer->isSecdBufFull()) 
				{
					// Get temp file index
					size_t temp_file_index = writer->getTempFileIndex((*(writer->secd_write_buffer.begin())).first);
					std::string temp_file_path = std::to_string(temp_file_index) + ".tmp";

					// Get the lock for temp file with that index
					file_locks[temp_file_index]->lock();

					std::fstream* input_stream = writer->memory->openFile(temp_file_path, "r");
					std::string new_temp_file_content = "";

					std::string url;
					uint64_t count;
					
					// For each line in that file, if there is a same URL in local buffer, we merge them and write the merged result to file
					while (*input_stream >> url >> count)
					{
						if (writer->secd_write_buffer.find(url) != writer->secd_write_buffer.end())
						{
							count += writer->secd_write_buffer[url];
							writer->secd_write_buffer.erase(url);
						}
						new_temp_file_content += url + " " + std::to_string(count) + "\n";
					}

					// Append all unvisited URL and their counts to end of the file
					for (auto unvisited_record : writer->secd_write_buffer)
					{
						new_temp_file_content += unvisited_record.first + " " + std::to_string(unvisited_record.second) + "\n";
					}

					writer->memory->closeFile(temp_file_path);
					input_stream = NULL;
					std::fstream* output_stream = writer->memory->openFile(temp_file_path, "w");
					(*output_stream) << new_temp_file_content;
					writer->memory->closeFile(temp_file_path);
					output_stream = NULL;
					writer->secd_write_buffer.clear();

					file_locks[temp_file_index]->unlock();
					
				}

				// Now the local buffer is empty, start to read records from main write buffer again until the next record is not in current temp file 
				// or main write buffer is empty
				// or local buffer is full
				std::pair<std::string, uint64_t> record = writer->memory->write_buffer.back();
				writer->memory->write_buffer.pop_back();
				writer->secd_write_buffer[record.first] = record.second;
				while (!writer->memory->isWriteBufferEmpty() && !writer->isSecdBufFull() &&
					writer->getTempFileIndex(writer->memory->write_buffer.back().first) == writer->getTempFileIndex(record.first))
				{
					record = writer->memory->write_buffer.back();
					writer->memory->write_buffer.pop_back();
					if (writer->secd_write_buffer.find(record.first) != writer->secd_write_buffer.end())
					{
						writer->secd_write_buffer[record.first] += record.second;
					}
					else
					{
						writer->secd_write_buffer[record.first] = record.second;
					}
				}
			}
			write_buffer_lock->unlock();
		}
	}

	void Writer::start()
	{
		std::thread write_thread(Writer::write, this);
		write_thread.detach();
	}

}

