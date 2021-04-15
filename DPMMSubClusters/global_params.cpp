#include "global_params.h"
#include "multinomial_prior.h"
#include "niw.h"

global_params::global_params(int numLabels, MatrixXd& all_data, unsigned long long randomSeed, prior_type priorType)
{
	if (priorType == prior_type::Gaussian)
	{
		pPrior = std::make_unique<niw>();
	}
	else if (priorType == prior_type::Multinomial)
	{
		pPrior = std::make_unique<multinomial_prior>();
	}
	//Data Loading specifics
	data_path = "/path/to/data/";
	data_prefix = "data_prefix"; //If the data file name is bob.npy, this should be 'bob'

	//Model Parameters
	iterations = 100;
	hard_clustering = false; //Soft or hard assignments
	initial_clusters = 1;
	argmax_sample_stop = 5;//Change to hard assignment from soft at iterations - argmax_sample_stop
	split_stop = 5;//Stop split / merge moves at  iterations - split_stop

	random_seed = randomSeed;//When nothing, a random seed will be used.
	rd = std::make_unique<std::random_device>(std::to_string(random_seed));
	std::random_device(std::to_string(random_seed));
	
	gen = std::make_unique<std::mt19937>((*rd)());

	max_split_iter = 20;
	burnout_period = 20;
	max_clusters = DBL_MAX;

	//Model hyperparams
	alpha = 10.0; //Concetration Parameter
	hyper_params = NULL;
	outlier_mod = 0.05; //Concetration Parameter
	outlier_hyper_params = NULL;

		//Saving specifics :
	enable_saving = true;
	model_save_interval = 1000;
	save_path = "/path/to/save/dir/";
	overwrite_prec = false;
	save_file_prefix = "checkpoint_";

	cuda = pPrior->get_cuda();
	cuda->init(numLabels, all_data, random_seed);
	numLabels = numLabels;
	points = all_data;
}

global_params::~global_params()
{
	if (cuda != NULL)
	{
		cuda->release();
	}
}
