#include <fstream>
#include <stdio.h>
#include "module_tests.h"
#include "jsonSerializer.h"

int main(int argc, char** argv)
{
	const std::string ParamsPath = "--params_path=";
	const std::string ModelPath = "--model_path=";
	const std::string LabelsPath = "--labels_path=";
	const std::string PriorType = "--prior_type=";
	const std::string ResultPath = "--result_path=";

    printf("main was run with %d arguments\n", argc-1);
    printf("Eigen uses %d threads\n", Eigen::nbThreads());
	std::string params_path;
	std::string model_path;
	std::string labels_path;
	std::string priortype;
	std::string result_path = "result.json";

	for (int i = 1; i < argc; ++i)
	{
		std::string s = argv[i];
		if (s.rfind(ParamsPath, 0) == 0)
		{
			params_path = s.substr(ParamsPath.size(), s.size() - ParamsPath.size());
			printf("Will look for params_path in:%s\n", params_path.c_str());
		}
		else if (s.rfind(ModelPath, 0) == 0)
		{
			model_path = s.substr(ModelPath.size(), s.size() - ModelPath.size());
			printf("Will look for model_path in:%s\n", model_path.c_str());
		}
		else if (s.rfind(LabelsPath, 0) == 0)
		{
			labels_path = s.substr(LabelsPath.size(), s.size() - LabelsPath.size());
			printf("Will look for labels_path in:%s\n", labels_path.c_str());
		}
		else if (s.rfind(PriorType, 0) == 0)
		{
			priortype = s.substr(PriorType.size(), s.size() - PriorType.size());
			printf("Prior is:%s\n", priortype.c_str());
		}
		else if (s.rfind(ResultPath, 0) == 0)
		{
			result_path = s.substr(ResultPath.size(), s.size() - ResultPath.size());
			printf("result_path is:%s\n", result_path.c_str());
		}
	}
	//model_path = "";
	if (model_path.size() == 0)
	{
		module_tests mt;
		mt.RandomMess();
	}
	else
	{
		prior_type pt = priortype.rfind("Multinomial", 0) == 0 ? prior_type::Multinomial : prior_type::Gaussian;
		dp_parallel_sampling_class dps(model_path, params_path, pt);
		ModelInfo dp = dps.dp_parallel_from_file();
		std::ofstream out(result_path);
		std::string output;
		int size = dp.labels->size();
		Json::Value root;
		for (int i = 0; i < size; i++)
		{
			root["labels"].append((*dp.labels)[i]);
		}
		size = dp.dp_model->group.weights.size();
		for (int i = 0; i < size; i++)
		{
			root["weights"].append(dp.dp_model->group.weights[i]);
		}

		size = dp.dp_model->group.points.size();
		double* data = dp.dp_model->group.points.data();
		for (int i = 0; i < size; i++)
		{
			root["points"].append(data[i]);
		}

		const int rows = dp.dp_model->group.points.rows();
		const int cols = dp.dp_model->group.points.cols();
		for (int i = 0; i < rows; i++)
		{
			std::string str = "points"+ std::to_string(i);
			for (int j = 0; j < cols; j++)
			{
				root[str.c_str()].append(dp.dp_model->group.points(i,j));
			}
		}

		size = dp.nmi_score_history.size();
		data = dp.nmi_score_history.data();
		for (int i = 0; i < size; i++)
		{
			root["nmi_score_history"].append(data[i]);
		}

		Json::StyledWriter writer;
		output = writer.write(root);
		out << output;
		out.close();
		
	}

	return EXIT_SUCCESS;
}