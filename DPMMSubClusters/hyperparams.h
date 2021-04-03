#pragma once
class hyperparams
{
public:
	virtual hyperparams* clone() = 0;
	virtual ~hyperparams() {}

};