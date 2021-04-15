#pragma once

#include <map>
#include "ds.h"
#include "global_params.h"

class local_clusters_actions
{
public:
	local_clusters_actions(std::shared_ptr<global_params>& globalParamsIn)
	{
		globalParams = globalParamsIn;
	}
	std::shared_ptr<local_cluster> create_first_local_cluster(local_group& group);
	std::shared_ptr<local_cluster> create_outlier_local_cluster(local_group& group, std::shared_ptr<hyperparams>& outlier_params);
	void group_step(local_group& group, bool no_more_splits, bool bFinal, bool first);
	void split_first_cluster_worker(local_group& group);
	void sample_clusters(local_group& group, bool first);
	void create_thin_cluster_params(std::vector<std::shared_ptr<local_cluster>>& clusters, std::vector<std::shared_ptr<thin_cluster_params>>& tcp);
	void broadcast_cluster_params(std::vector<std::shared_ptr<thin_cluster_params>>& params_vector, std::vector<double>& weights_vector);
	void set_global_data(const std::vector<std::shared_ptr<thin_cluster_params>>& params_vector, const std::vector<double>& weights_vector);
	void sample_labels(local_group& group, bool bFinal, bool no_more_splits);
	void sample_sub_clusters(local_group& group);
	void update_suff_stats_posterior(local_group& group);
	void update_suff_stats_posterior(local_group& group, LabelsType& indices, bool indicesValid);
	std::map<LabelType, std::shared_ptr<thin_suff_stats>> create_suff_stats_dict_worker(MatrixXd& group_pts, std::shared_ptr<hyperparams>& hyper_params, LabelsType& indices);
	void update_splittable_cluster_params(std::shared_ptr<splittable_cluster_params>& splittable_cluser);
	void reset_bad_clusters(local_group& group);
	void reset_bad_clusters_worker(LabelsType& indices, MatrixXd& group_points);
	void check_and_split(local_group& group, bool bFinal, LabelsType& all_indices);
	void should_split_local(double& should_split, std::shared_ptr<splittable_cluster_params>& cluster_params, double alpha, bool bFinal);
	void split_cluster_local(local_group& group, std::shared_ptr<local_cluster>& cluster, ClusterIndexType index, ClusterIndexType new_index);
	void split_cluster_local_worker(LabelsType& indices, LabelsType& new_indices);
	void merge_clusters_worker(LabelsType& indices, LabelsType& new_indices);
	void check_and_merge(local_group& group, bool bFinal);
	void merge_clusters(local_group& group, ClusterIndexType index_l, ClusterIndexType index_r);
	void remove_empty_clusters(local_group& group);
	void remove_empty_clusters_worker(std::vector<PointType>& pts_count);

protected:
	virtual std::vector<double> get_dirichlet_distribution(std::vector<double>& points_count);

private:
	std::shared_ptr<global_params> globalParams;
};

