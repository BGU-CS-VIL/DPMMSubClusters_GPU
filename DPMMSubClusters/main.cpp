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

	printf("main was run with %d arguments\n", argc - 1);
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
		std::ofstream out(result_path);
		Json::Value root;
		Json::StreamWriterBuilder builder;
		std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
		try
		{
			prior_type pt = priortype.rfind("Multinomial", 0) == 0 ? prior_type::Multinomial : prior_type::Gaussian;
			dp_parallel_sampling_class dps(model_path, params_path, pt);
			ModelInfo dp = dps.dp_parallel_from_file();
			size_t size = dp.labels->size();
			double* data = NULL;

			size = dp.nmi_score_history.size();
			data = dp.nmi_score_history.data();
			for (size_t i = 0; i < size; i++)
			{
				root["nmi_score_history"].append(data[i]);
			}

			size = dp.iter_count.size();
			data = dp.iter_count.data();
			for (size_t i = 0; i < size; i++)
			{
				root["iter_count"].append(data[i]);
			}

			//Below can be enabled if needed. But it might take time to save it to the file.
			//for (size_t i = 0; i < size; i++)
			//{
			//	root["labels"].append((*dp.labels)[i]);
			//}
			//size = dp.dp_model->group.weights.size();
			//for (int i = 0; i < size; i++)
			//{
			//	root["weights"].append(dp.dp_model->group.weights[i]);
			//}

			//size = dp.dp_model->group.points.size();
			//data = dp.dp_model->group.points.data();
			//for (size_t i = 0; i < size; i++)
			//{
			//	root["points"].append(data[i]);
			//}

			//const size_t rows = dp.dp_model->group.points.rows();
			//const size_t cols = dp.dp_model->group.points.cols();
			//for (size_t i = 0; i < rows; i++)
			//{
			//	std::string str = "points" + std::to_string(i);
			//	for (size_t j = 0; j < cols; j++)
			//	{
			//		root[str.c_str()].append(dp.dp_model->group.points(i, j));
			//	}
			//}
			writer->write(root, &out);
		}
		catch (const std::exception& e)
		{
			printf("exception:%s\n", e.what());
			root["error"].append(e.what());
			writer->write(root, &out);

			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}