#pragma once

#include <vector>
#include "distribution_sample.h"
#include "moduleTypes.h"
#include "Eigen/Dense"
#include "hyperparams.h"
#include "sufficient_statistics.h"

using namespace Eigen;

struct model_hyper_params : public IJsonSerializable
{
	model_hyper_params() {}
	model_hyper_params(hyperparams* distribution_hyper_params, double alpha, DimensionsType total_dim) : distribution_hyper_params(distribution_hyper_params), alpha(alpha), total_dim(total_dim) {}
	virtual void serialize(Json::Value& root)
	{
		Json::Value distribution_hyper_params_val;
		distribution_hyper_params->serialize(distribution_hyper_params_val);
		root["distribution_hyper_params"] = distribution_hyper_params_val;

		root["alpha"] = alpha;
		root["total_dim"] = total_dim;
	}
	virtual void deserialize(Json::Value& root)
	{
		//TODO
	}
	
	std::shared_ptr<hyperparams> distribution_hyper_params;
	double alpha;
	DimensionsType total_dim;
	friend std::ostream& operator<<(std::ostream& os, const model_hyper_params& mhp)
	{
		os << "{distribution_hyper_params: " << mhp.distribution_hyper_params <<
			" || alpha: " << mhp.alpha <<
			" || total_dim: " << mhp.total_dim <<
			"}" << std::endl;
		return os;
	}
};

class cluster_parameters : public IJsonSerializable
{
public:
	cluster_parameters() : prior_hyperparams(NULL), distribution(NULL), suff_statistics(NULL), posterior_hyperparams(NULL) {}

	cluster_parameters(const cluster_parameters &cp)
	{
		copy(cp);
	}

	std::shared_ptr<cluster_parameters> clone()
	{
		std::shared_ptr<cluster_parameters> cp = std::make_shared<cluster_parameters>();
		cp->copy(*this);
		return cp;
	}

	void copy(const cluster_parameters &cp)
	{
		if (cp.prior_hyperparams != NULL)
		{
			prior_hyperparams = cp.prior_hyperparams->clone();
		}
		else
		{
			prior_hyperparams = NULL;
		}

		if (cp.distribution != NULL)
		{
			distribution = cp.distribution->clone();
		}
		else
		{
			distribution = NULL;
		}

		if (cp.suff_statistics != NULL)
		{
			suff_statistics = cp.suff_statistics->clone();
		}
		else
		{
			suff_statistics = NULL;
		}

		if (cp.posterior_hyperparams != NULL)
		{
			posterior_hyperparams = cp.posterior_hyperparams->clone();
		}
		else
		{
			posterior_hyperparams = NULL;
		}
	}

	virtual void serialize(Json::Value& root)
	{
		Json::Value prior_hyperparams_val;
		prior_hyperparams->serialize(prior_hyperparams_val);
		root["prior_hyperparams"] = prior_hyperparams_val;

		Json::Value distribution_val;
		distribution->serialize(distribution_val);
		root["distribution"] = distribution_val;

		Json::Value suff_statistics_val;
		suff_statistics->serialize(suff_statistics_val);
		root["suff_statistics"] = suff_statistics_val;

		Json::Value posterior_hyperparams_val;
		posterior_hyperparams->serialize(posterior_hyperparams_val);
		root["posterior_hyperparams"] = posterior_hyperparams_val;
	}
	virtual void deserialize(Json::Value& root)
	{
		//TODO
	}

	std::shared_ptr<hyperparams> prior_hyperparams;
	std::shared_ptr<distribution_sample> distribution;
	std::shared_ptr<sufficient_statistics> suff_statistics;
	std::shared_ptr<hyperparams> posterior_hyperparams;
};

typedef std::vector<std::pair<bool, double>> Logsublikelihood_hist;

class splittable_cluster_params : public IJsonSerializable
{
public:
	splittable_cluster_params() : cluster_params(NULL), cluster_params_l(NULL), cluster_params_r(NULL), splittable(false) {}

	splittable_cluster_params(const splittable_cluster_params &scp)
	{
		copy(scp);
	}

	std::shared_ptr<splittable_cluster_params> clone()
	{
		std::shared_ptr<splittable_cluster_params> scp = std::make_shared<splittable_cluster_params>();
		scp->copy(*this);
		return scp;
	}

	void copy(const splittable_cluster_params &scp)
	{
		if (scp.cluster_params != NULL)
		{
			cluster_params = scp.cluster_params->clone();
		}
		else
		{
			cluster_params = NULL;
		}

		if (scp.cluster_params_l != NULL)
		{
			cluster_params_l = scp.cluster_params_l->clone();
		}
		else
		{
			cluster_params_l = NULL;
		}

		if (scp.cluster_params_r != NULL)
		{
			cluster_params_r = scp.cluster_params_r->clone();
		}
		else
		{
			cluster_params_r = NULL;
		}

		lr_weights = scp.lr_weights;
		splittable = scp.splittable;
		logsublikelihood_hist = scp.logsublikelihood_hist;
	}

	virtual void serialize(Json::Value& root)
	{
		Json::Value cluster_params_val;
		cluster_params->serialize(cluster_params_val);
		root["cluster_params"] = cluster_params_val;

		Json::Value cluster_params_l_val;
		cluster_params_l->serialize(cluster_params_l_val);
		root["cluster_params_l"] = cluster_params_l_val;

		Json::Value cluster_params_r_val;
		cluster_params_r->serialize(cluster_params_r_val);
		root["cluster_params_r"] = cluster_params_r_val;

		int size = lr_weights.size();
		for (int i = 0; i < size; i++)
		{
			root["lr_weights"].append(lr_weights[i]);
		}

		root["splittable"] = splittable;

		size = logsublikelihood_hist.size();
		for (int i = 0; i < size; i++)
		{
			Json::Value pair;
			pair.append(logsublikelihood_hist[i].first);
			pair.append(logsublikelihood_hist[i].second);
			root["logsublikelihood_hist"].append(pair);
		}

	}
	virtual void deserialize(Json::Value& root)
	{
		//TODO
	}

	std::shared_ptr<cluster_parameters> cluster_params;
	std::shared_ptr<cluster_parameters> cluster_params_l;
	std::shared_ptr<cluster_parameters> cluster_params_r;
	std::vector<double> lr_weights;
	bool splittable;
	Logsublikelihood_hist logsublikelihood_hist;//bool - true is set. false not set
};

class thin_cluster_params
{
public:
	thin_cluster_params(std::shared_ptr<distribution_sample> cluster_dist, std::shared_ptr<distribution_sample> l_dist, std::shared_ptr<distribution_sample> r_dist, const std::vector<double> &lr_weights):cluster_dist(cluster_dist), l_dist(l_dist), r_dist(r_dist), lr_weights(lr_weights)
	{
	}
	~thin_cluster_params()
	{
	}
	std::shared_ptr<distribution_sample> cluster_dist;
	std::shared_ptr<distribution_sample> l_dist;
	std::shared_ptr<distribution_sample> r_dist;
	std::vector<double> lr_weights;//between 0 to 1
};

struct thin_suff_stats
{
	std::shared_ptr<sufficient_statistics> cluster_suff;
	std::shared_ptr<sufficient_statistics> l_suff;
	std::shared_ptr<sufficient_statistics> r_suff;
};

struct local_cluster : public IJsonSerializable
{
	local_cluster() : cluster_params(NULL), total_dim(0), points_count(0), l_count(0), r_count(0) {}

	~local_cluster()
	{
	}

	local_cluster(const std::shared_ptr<local_cluster>& lc)
	{
		copy(lc);
	}

	std::shared_ptr<local_cluster> clone()
	{
		std::shared_ptr<local_cluster> lc = std::make_shared<local_cluster>();
		lc->copy(*this);
		return lc;
	}

	local_cluster& operator=(local_cluster other)
	{
		copy(other);
		return *this;
	}

	void copy(const local_cluster &lc)
	{
		if (lc.cluster_params != NULL)
		{
			cluster_params = lc.cluster_params->clone();
		}
		else
		{
			cluster_params = NULL;
		}

		total_dim = lc.total_dim;
		points_count = lc.points_count;
		l_count = lc.l_count;
		r_count = lc.r_count;
	}
	virtual void serialize(Json::Value& root)
	{
		Json::Value cluster_params_val;
		cluster_params->serialize(cluster_params_val);
		root["cluster_params"] = cluster_params_val;

		root["total_dim"] = total_dim;
		root["points_count"] = points_count;
		root["l_count"] = l_count;
		root["r_count"] = r_count;

	}
	virtual void deserialize(Json::Value& root)
	{
		//TODO
	}

	std::shared_ptr<splittable_cluster_params> cluster_params;
	DimensionsType total_dim;
	PointType points_count;
	LabelType l_count;
	LabelType r_count;
};

struct local_group : public IJsonSerializable
{
	local_group() {}
	local_group(model_hyper_params model_hyperparams, MatrixXd points, LabelsType &labels_subcluster, std::vector<std::shared_ptr<local_cluster>>& local_clusters, std::vector<double> weights) :
		model_hyperparams(model_hyperparams), points(points), local_clusters(local_clusters), weights(weights) {}
	virtual void serialize(Json::Value& root)
	{
		Json::Value model_hyperparams_val;
		model_hyperparams.serialize(model_hyperparams_val);
		root["model_hyperparams"] = model_hyperparams_val;

		int size = points.size();
		double* data = points.data();
		for (int i = 0; i < size; i++)
		{
			root["points"].append(data[i]);
		}

		size = local_clusters.size();
		for (int i = 0; i < size; i++)
		{
			Json::Value local_cluster_val;
			local_clusters[i]->serialize(local_cluster_val);
			root["local_clusters"].append(local_cluster_val);
		}

		size = weights.size();
		for (int i = 0; i < size; i++)
		{
			root["weights"].append(weights[i]);
		}
	}
	virtual void deserialize(Json::Value& root)
	{
		//TODO
	}

	LabelType num_labels()
	{
		return (LabelType)points.cols();
	}

	model_hyper_params model_hyperparams;
	MatrixXd points;
	std::vector<std::shared_ptr<local_cluster>> local_clusters;
	std::vector<double> weights;
};

//
class pts_less_group
{
public:
	pts_less_group(model_hyper_params model_hyperparam, std::vector<std::shared_ptr<local_cluster>>& local_clusters, std::vector<double>& weights) :
		model_hyperparams(model_hyperparams), local_clusters(local_clusters), weights(weights) {}
	model_hyper_params model_hyperparams;
	std::vector<std::shared_ptr<local_cluster>> local_clusters;
	std::vector<double> weights;

	friend std::ostream& operator<<(std::ostream& os, const pts_less_group& plg)
	{
		os << "{model_hyperparams: " << plg.model_hyperparams <<
			//" || labels: " << plg.labels.size() <<
			//" || labels_subcluster: " << plg.labels_subcluster.size() <<
			" || local_clusters: " << plg.local_clusters.size() <<
			" || weights: " << plg.weights.size() <<
			"}" << std::endl;
		return os;
	}
};

struct dp_parallel_sampling : public IJsonSerializable
{
	dp_parallel_sampling() {}
	dp_parallel_sampling(model_hyper_params model_hyperparams, local_group &group) :model_hyperparams(model_hyperparams), group(group) {}
	virtual void serialize(Json::Value& root)
	{
		Json::Value model_hyperparams_val;
		model_hyperparams.serialize(model_hyperparams_val);
		root["model_hyperparams"] = model_hyperparams_val;

		Json::Value group_val;
		group.serialize(group_val);
		root["group"] = group_val;
	}
	virtual void deserialize(Json::Value& root) 
	{
		//TODO
	}
	
	model_hyper_params model_hyperparams;
	local_group group;
};

class ds
{
public:
	static pts_less_group* create_pts_less_group(local_group &group)
	{
		return new pts_less_group(group.model_hyperparams, group.local_clusters, group.weights);
	}
};