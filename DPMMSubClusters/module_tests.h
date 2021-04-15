#pragma once

#include "dp_parallel_sampling.h"

class module_tests
{
public:
	ClusterIndexType RandomMess();
	ClusterIndexType RandomMessHighDim();
	void TestingData(LabelsType&labels);
	void ReadPnyFileIntoData(std::string path, std::string prefix, MatrixXd &mat);
	ClusterIndexType RunModuleFromFile(std::string path, std::string prefix);
	void CheckMemoryLeak();

};

