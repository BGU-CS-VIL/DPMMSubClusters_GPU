#pragma once
#include <memory>
#include "IJsonSerializable.h"

class hyperparams : public IJsonSerializable
{
public:
	virtual std::shared_ptr<hyperparams> clone() = 0;
	virtual ~hyperparams() {}
	virtual void serialize(Json::Value& root) = 0;
};