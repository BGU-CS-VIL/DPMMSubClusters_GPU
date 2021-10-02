#pragma once
#include "IJsonSerializable.h"

class CJsonSerializer
{
public:
	static bool serialize(IJsonSerializable* pObj, std::string& output);

private:
	CJsonSerializer() {};
};

