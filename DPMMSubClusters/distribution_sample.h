#pragma once

#include <memory>
#include "IJsonSerializable.h"

class distribution_sample : IJsonSerializable
{
public:
	virtual ~distribution_sample() {}
	virtual void serialize(Json::Value& root) = 0;
	virtual void deserialize(Json::Value& root) = 0;

	virtual std::shared_ptr<distribution_sample> clone() = 0;
};

