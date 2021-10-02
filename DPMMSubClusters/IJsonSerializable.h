#pragma once

#include <json/json.h>

class IJsonSerializable
{
public:
	virtual ~IJsonSerializable() {};
	virtual void serialize(Json::Value& root) = 0;
};

