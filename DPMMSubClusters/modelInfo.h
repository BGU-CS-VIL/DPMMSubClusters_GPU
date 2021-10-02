#pragma once

#include <vector>
#include "IJsonSerializable.h"
#include "ds.h"
#include "global_params.h"

class ModelInfo : public IJsonSerializable
{
public:
	ModelInfo() : dp_model(nullptr), labels(std::make_shared<LabelsType>()){}
	virtual void serialize(Json::Value& root)
	{
		// serialize primitives
		Json::Value dp_model_val;
		dp_model->serialize(dp_model_val);
		root["dp_model"] = dp_model_val;

		size_t size = iter_count.size();
		for (size_t i = 0; i < size; i++)
		{
			root["iter_count"].append(iter_count[i]);
		}

		size = nmi_score_history.size();
		for (size_t i = 0; i < size; i++)
		{
			root["nmi_score_history"].append(nmi_score_history[i]);
		}

		size = likelihood_history.size();
		for (size_t i = 0; i < size; i++)
		{
			root["likelihood_history"].append(likelihood_history[i]);
		}

		size = cluster_count_history.size();
		for (size_t i = 0; i < size; i++)
		{
			root["cluster_count_history"].append(cluster_count_history[i]);
		}
	}

	//	- `dp_model` The DPMM model inferred
	std::shared_ptr<dp_parallel_sampling> dp_model;
	//`iter_count` Timing for each iteration
	std::vector<double> iter_count;
	//	- `nmi_score_history` NMI score per iteration(if gt suppled)
	std::vector<double> nmi_score_history;
	//	- `likelihood_history` Log likelihood per iteration.
	std::vector<double> likelihood_history;

	//	- `cluster_count_history` Cluster counts per iteration.
	std::vector<ClusterIndexType> cluster_count_history;

	std::shared_ptr<LabelsType> labels;
};