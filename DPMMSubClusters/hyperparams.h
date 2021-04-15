#pragma once
#include <memory>

class hyperparams
{
public:
	virtual std::shared_ptr<hyperparams> clone() = 0;
	virtual ~hyperparams() {}

};