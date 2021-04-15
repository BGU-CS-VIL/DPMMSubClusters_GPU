#pragma once

#include <vector>
#include "distribution_sample.h"
#include "moduleTypes.h"
#include "Eigen/Dense"
#include "hyperparams.h"
#include "sufficient_statistics.h"

using namespace Eigen;

struct model_hyper_params
{
	model_hyper_params() {}
	model_hyper_params(hyperparams* distribution_hyper_params, double alpha, DimensionsType total_dim) : distribution_hyper_params(distribution_hyper_params), alpha(alpha), total_dim(total_dim) {}
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

class cluster_parameters
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

	std::shared_ptr<hyperparams> prior_hyperparams;
	std::shared_ptr<distribution_sample> distribution;
	std::shared_ptr<sufficient_statistics> suff_statistics;
	std::shared_ptr<hyperparams> posterior_hyperparams;
};

typedef std::vector<std::pair<bool, double>> Logsublikelihood_hist;

class splittable_cluster_params
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
	thin_cluster_params(std::shared_ptr<distribution_sample>& cluster_dist, std::shared_ptr<distribution_sample>& l_dist, std::shared_ptr<distribution_sample>& r_dist, const std::vector<double> &lr_weights):cluster_dist(cluster_dist), l_dist(l_dist), r_dist(r_dist), lr_weights(lr_weights)
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

struct local_cluster
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

	std::shared_ptr<splittable_cluster_params> cluster_params;
	DimensionsType total_dim;
	PointType points_count;
	LabelType l_count;
	LabelType r_count;
};

struct local_group
{
	local_group() {}
	local_group(model_hyper_params model_hyperparams, MatrixXd points, LabelsType &labels_subcluster, std::vector<std::shared_ptr<local_cluster>>& local_clusters, std::vector<double> weights) :
		model_hyperparams(model_hyperparams), points(points), local_clusters(local_clusters), weights(weights) {}
	
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

struct dp_parallel_sampling
{
	dp_parallel_sampling() {}
	dp_parallel_sampling(model_hyper_params model_hyperparams, local_group &group) :model_hyperparams(model_hyperparams), group(group) {}

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