#pragma once
#include "IJsonSerializable.h"

class CJsonSerializer
{
public:
	static bool serialize(IJsonSerializable* pObj, std::string& output);
	static bool deserialize(IJsonSerializable* pObj, std::string& input);

private:
	CJsonSerializer(void) {};
};

