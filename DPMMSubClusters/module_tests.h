#pragma once

#include "dp_parallel_sampling.h"

class module_tests
{
public:
	ClusterIndexType RandomMess();
	ClusterIndexType RandomMessHighDim();
	void TestingData(LabelsType&labels);
	void CheckMemoryLeak();

};

