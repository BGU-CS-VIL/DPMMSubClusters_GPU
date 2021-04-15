#pragma once
class hyperparams
{
public:
	virtual std::shared_ptr<hyperparams> clone() = 0;
	virtual ~hyperparams() {}

};