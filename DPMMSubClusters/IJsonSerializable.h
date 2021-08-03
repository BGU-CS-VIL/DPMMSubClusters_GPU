#pragma once

#include <json/json.h>

class IJsonSerializable
{
public:
	virtual ~IJsonSerializable(void) {};
	virtual void serialize(Json::Value& root) = 0;
	virtual void deserialize(Json::Value& root) = 0;
};

