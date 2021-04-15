#pragma once

#include <memory>

class distribution_sample
{
public:
	virtual ~distribution_sample() {}

	virtual std::shared_ptr<distribution_sample> clone() = 0;
};

