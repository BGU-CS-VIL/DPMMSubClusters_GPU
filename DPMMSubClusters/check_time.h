#pragma once
#include <iostream>
#include <string>
#include <chrono>

#define CHECK_TIME(msg) check_time auto_check_time(msg);

class check_time
{
public:
	check_time(std::string msg)
	{
		message = msg;
		begin = std::chrono::steady_clock::now();
	}

	~check_time()
	{
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		long long took = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
		if (took > 0)
		{
			std::cout << message << " took:" << took << "[seconds]" << std::endl;
		}
	}

private:
	std::string message;
	std::chrono::steady_clock::time_point begin;
};

