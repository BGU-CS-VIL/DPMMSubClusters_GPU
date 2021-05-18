#pragma once
#include <iostream>
#include <string>
//#include <chrono>
#include <ctime>

using namespace std;

class check_time
{
public:
	check_time(std::string msg)
	{
		message = msg;
		begin = clock();
	}

	~check_time()
	{
		clock_t end = clock();
		double took = double(end - begin) / CLOCKS_PER_SEC;
		if (took > 0.1)
		{
			std::cout << message << " took:" << took << "[seconds]" << std::endl;
		}
	}

private:
	std::string message;
	clock_t begin;
};

#define CHECK_TIME(msg) //check_time auto_check_time(msg);