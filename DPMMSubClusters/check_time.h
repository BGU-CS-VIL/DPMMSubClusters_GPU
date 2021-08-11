#pragma once
#include <iostream>
#include <string>
#include <ctime>

using namespace std;

class check_time
{
public:
	check_time(std::string msg, bool use_verbose)
	{
		verbose = use_verbose;
		if (verbose)
		{
			message = msg;
			begin = clock();
		}
	}

	~check_time()
	{
		if (verbose)
		{
			clock_t end = clock();
			double took = double(end - begin) / CLOCKS_PER_SEC;
			if (took > 0.1)
			{
				std::cout << message << " took:" << took << "[seconds]" << std::endl;
			}
		}
	}

private:
	std::string message;
	clock_t begin;
	bool verbose;
};

#define CHECK_TIME(msg, use_verbose) check_time auto_check_time(msg, use_verbose);