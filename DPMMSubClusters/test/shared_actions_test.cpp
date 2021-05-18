#include <locale>
#include <codecvt>
#include <string>
#include "pch.h"
#include "gtest/gtest.h"
#include "priors/niw.h"
#include "Eigen/Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"
#include "local_clusters_actions.h"
#include "shared_actions.h"
#include "myGen.h"
#include "myCudaKernel.h"
#include "myNiw.h"
#include "priors/niw_hyperparams.h"
#include "priors/niw_sufficient_statistics.h"

namespace DPMMSubClustersTest
{
	double* get_double_array(double a, double b)
	{
		double* result = new double[2];
		result[0] = a;
		result[1] = b;
		return result;
	}

	class myShared_actions : public shared_actions
	{
	public:
		myShared_actions(std::shared_ptr<global_params> &globalParamsIn) :shared_actions(globalParamsIn)
		{
			dirichletIter = 0;
		}

		void set_dirichlet_distribution_values(std::vector<std::vector<double>>& values)
		{
			dirichlet_distribution_values = values;
		}

		void set_uniform_real_distribution(double value)
		{
			uniform_real_distribution = value;
		}

	protected:
		virtual std::vector<double> get_dirichlet_distribution(std::vector<double>& points_count)
		{
			++dirichletIter;
			if (dirichlet_distribution_values.size() < dirichletIter)
			{
				std::vector<double> result;
				for (int i = 0; i < points_count.size(); i++)
				{
					result.push_back(1.0 / points_count.size());
				}

				return result;
			}
			else
			{
				return dirichlet_distribution_values[dirichletIter - 1];
			}
		}

		virtual double get_uniform_real_distribution()
		{
			return uniform_real_distribution;
		}

	private:
		int dirichletIter;
		std::vector<std::vector<double>> dirichlet_distribution_values;
		double uniform_real_distribution;
	};


	TEST(shared_actions_test, sample_cluster_params)
	{
		MatrixXd points;
		std::unique_ptr<myCudaKernel_gaussian> myCudaKernelObj = std::make_unique<myCudaKernel_gaussian>();
		std::shared_ptr<global_params> gp = std::make_shared<global_params>();
		gp->init(10, points, NULL, prior_type::Gaussian);
		gp->cuda = std::move(myCudaKernelObj);
		gp->burnout_period = 15;
		myShared_actions object(gp);

		VectorXd m(2);
		m << 0.0, 0.0;
		MatrixXd psi(2, 2);
		psi << 1.0, 0.0, 0.0, 1.0;
		myNiw* prior = new myNiw();
		niw_hyperparams niwHyperparams(1.0, m, 5.0, psi);

		std::shared_ptr<splittable_cluster_params> cluster_params = std::make_shared<splittable_cluster_params>();
		//Both
		cluster_params->cluster_params = std::make_shared<cluster_parameters>();
		cluster_params->cluster_params->prior_hyperparams = niwHyperparams.clone();
		std::shared_ptr<mv_gaussian> mvGaussian = std::make_shared<mv_gaussian>();
		mvGaussian->mu = VectorXd(2);
		mvGaussian->mu << -0.48733974, 1.2147124;
		mvGaussian->sigma = MatrixXd(2, 2);
		mvGaussian->sigma << 0.6517296, -0.1934098, -0.1934098, 0.48445275;
		mvGaussian->invSigma = MatrixXd(2, 2);
		mvGaussian->invSigma << 1.7406019, 0.69490665, 0.69490665, 2.3416147;
		mvGaussian->logdetSigma = -1.2789663;
		cluster_params->cluster_params->distribution = mvGaussian;
		std::shared_ptr<niw_sufficient_statistics> suff_statistics = std::make_shared<niw_sufficient_statistics>();
		suff_statistics->N = 10;
		suff_statistics->points_sum = VectorXd(2);
		suff_statistics->points_sum << 34.297726, -8.619593;
		suff_statistics->S = MatrixXd(2, 2);
		suff_statistics->S << 598.17426, -270.93265, -270.93265, 1169.5474;
		cluster_params->cluster_params->suff_statistics = suff_statistics;
		VectorXd m1(2);
		m1 << 3.117975, -0.7835993;
		MatrixXd psi1(2, 2);
		psi1 << 33.08232, -16.270466, -16.270466, 77.852875;
		niw_hyperparams niwHyperparams1(11.0, m1, 15.0, psi1);
		cluster_params->cluster_params->posterior_hyperparams = niwHyperparams1.clone();

		//Left
		cluster_params->cluster_params_l = std::make_shared<cluster_parameters>();
		cluster_params->cluster_params_l->prior_hyperparams = niwHyperparams.clone();
		mvGaussian = std::make_shared<mv_gaussian>();
		mvGaussian->mu = VectorXd(2);
		mvGaussian->mu << -0.48733974, 1.2147124;
		mvGaussian->sigma = MatrixXd(2, 2);
		mvGaussian->sigma << 0.6517296, -0.1934098, -0.1934098, 0.48445275;
		mvGaussian->invSigma = MatrixXd(2, 2);
		mvGaussian->invSigma << 1.7406019, 0.69490665, 0.69490665, 2.3416147;
		mvGaussian->logdetSigma = -1.2789663;

		cluster_params->cluster_params_l->distribution = mvGaussian;
		suff_statistics = std::make_shared<niw_sufficient_statistics>();
		suff_statistics->N = 6;
		suff_statistics->points_sum = VectorXd(2);
		suff_statistics->points_sum << 21.88504, -11.220918;
		suff_statistics->S = MatrixXd(2, 2);
		suff_statistics->S << 494.728, -224.80331, -224.80331, 824.1572;
		cluster_params->cluster_params_l->suff_statistics = suff_statistics;
		VectorXd m2(2);
		m2 << 3.1264343, -1.6029882;
		MatrixXd psi2(2, 2);
		psi2 << 39.20962, -17.24744, -17.24744, 73.742744;
		niw_hyperparams niwHyperparams2(7.0, m2, 11.0, psi2);
		cluster_params->cluster_params_l->posterior_hyperparams = niwHyperparams2.clone();

		//Right
		cluster_params->cluster_params_r = std::make_shared<cluster_parameters>();
		cluster_params->cluster_params_r->prior_hyperparams = niwHyperparams.clone();
		mvGaussian = std::make_shared<mv_gaussian>();
		mvGaussian->mu = VectorXd(2);
		mvGaussian->mu << -0.48733974, 1.2147124;
		mvGaussian->sigma = MatrixXd(2, 2);
		mvGaussian->sigma << 0.6517296, -0.1934098, -0.1934098, 0.48445275;
		mvGaussian->invSigma = MatrixXd(2, 2);
		mvGaussian->invSigma << 1.7406019, 0.69490665, 0.69490665, 2.3416147;
		mvGaussian->logdetSigma = -1.2789663;

		cluster_params->cluster_params_r->distribution = mvGaussian;
		suff_statistics = std::make_shared<niw_sufficient_statistics>();
		suff_statistics->N = 4;
		suff_statistics->points_sum = VectorXd(2);
		suff_statistics->points_sum << 12.412685, 2.601328;
		suff_statistics->S = MatrixXd(2, 2);
		suff_statistics->S << 103.4463, -46.129356, -46.129356, 345.3901;
		cluster_params->cluster_params_r->suff_statistics = suff_statistics;
		VectorXd m3(2);
		m3 << 2.482537, 0.5202656;
		MatrixXd psi3(2, 2);
		psi3 << 8.625706, -5.8430276, -5.8430276, 38.781857;
		niw_hyperparams niwHyperparams3(5.0, m3, 9, psi3);
		cluster_params->cluster_params_r->posterior_hyperparams = niwHyperparams3.clone();
		cluster_params->logsublikelihood_hist = Logsublikelihood_hist(gp->burnout_period + 5, std::make_pair(false, 0));

		std::vector<std::vector<double>> dirichlet_distribution_values;
		dirichlet_distribution_values.push_back({ 0.39355472, 0.60644525 });
		dirichlet_distribution_values.push_back({ 0.5560981, 0.44390193 });
		dirichlet_distribution_values.push_back({ 0.7682158, 0.23178421 });
		dirichlet_distribution_values.push_back({ 0.57142407, 0.42857596 });
		dirichlet_distribution_values.push_back({ 0.424758, 0.575242 });
		dirichlet_distribution_values.push_back({ 0.632061, 0.36793897 });
		dirichlet_distribution_values.push_back({ 0.6433123, 0.35668772 });
		dirichlet_distribution_values.push_back({ 0.4751574, 0.5248426 });
		dirichlet_distribution_values.push_back({ 0.5433636, 0.45663643 });
		dirichlet_distribution_values.push_back({ 0.4362832, 0.5637168 });
		dirichlet_distribution_values.push_back({ 0.29947805, 0.70052195 });
		dirichlet_distribution_values.push_back({ 0.4860855, 0.51391447 });
		dirichlet_distribution_values.push_back({ 0.558281, 0.441719 });
		dirichlet_distribution_values.push_back({ 0.50846565, 0.49153435 });
		dirichlet_distribution_values.push_back({ 0.5882769, 0.41172308 });
		object.set_dirichlet_distribution_values(dirichlet_distribution_values);

		std::vector<MatrixXd> inverseWishart_values;
		MatrixXd mat(2, 2);
		mat << 36.14617235356051, -19.341588666689596, -19.341588666689596, 68.87630555273348;
		inverseWishart_values.push_back(mat);
		mat << 30.899730372976133, -10.439212870730575, -10.439212870730575, 60.010035950085566;
		inverseWishart_values.push_back(mat);
		mat << 12.109655371385188, -22.316125195236268, -22.316125195236268, 91.44036632639707;
		inverseWishart_values.push_back(mat);
		mat << 34.30945433744633, -44.18068044591819, -44.18068044591819, 137.13926467066102;
		inverseWishart_values.push_back(mat);
		mat << 30.00108254676632, -10.416029308663106, -10.416029308663106, 124.11246650793389;
		inverseWishart_values.push_back(mat);
		mat << 28.27819988788713, -50.97291164220868, -50.97291164220868, 150.91671669155045;
		inverseWishart_values.push_back(mat);
		mat << 32.07642931260924, -0.8922360982837433, -0.8922360982837433, 48.19319058266629;
		inverseWishart_values.push_back(mat);
		mat << 52.432327035426596, 7.694299942802333, 7.694299942802333, 70.22220379935058;
		inverseWishart_values.push_back(mat);
		mat << 4.204833818350024, 4.026866663530245, 4.026866663530245, 9.353006881767827;
		inverseWishart_values.push_back(mat);
		mat << 16.032162329978675, -11.20457365077398, -11.20457365077398, 98.63103094987677;
		inverseWishart_values.push_back(mat);
		mat << 73.24545680684528, -71.48345910648672, -71.48345910648672, 89.78819228401932;
		inverseWishart_values.push_back(mat);
		mat << 5.48833259740756, 9.140655233307324, 9.140655233307324, 18.49159680272469;
		inverseWishart_values.push_back(mat);
		mat << 27.088090077960526, -13.465773881644408, -13.465773881644408, 57.846659384412;
		inverseWishart_values.push_back(mat);
		mat << 41.944006991087946, -30.702961278892364, -30.702961278892364, 46.13248086237989;
		inverseWishart_values.push_back(mat);
		mat << 7.252253387460507, 8.430643304953406, 8.430643304953406, 14.4361657821449;
		inverseWishart_values.push_back(mat);
		mat << 18.458900915907527, -5.888679187630051, -5.888679187630051, 40.25873055396536;
		inverseWishart_values.push_back(mat);
		mat << 70.75098021400638, -65.10024103494708, -65.10024103494708, 87.10531591635952;
		inverseWishart_values.push_back(mat);
		mat << 8.081478206619568, 11.138954220541168, 11.138954220541168, 18.563899905282486;
		inverseWishart_values.push_back(mat);
		mat << 50.368528350274964, -23.075578062329043, -23.075578062329043, 104.99019468862073;
		inverseWishart_values.push_back(mat);
		mat << 45.72744261291319, -29.880334643159227, -29.880334643159227, 29.781713064474815;
		inverseWishart_values.push_back(mat);
		mat << 11.1221600450625, 11.4423156217925, 11.4423156217925, 14.362100821247793;
		inverseWishart_values.push_back(mat);
		mat << 23.149688587881304, -16.765344640194243, -16.765344640194243, 97.91642976369788;
		inverseWishart_values.push_back(mat);
		mat << 38.50540830941135, -50.59143801980417, -50.59143801980417, 98.51420708067444;
		inverseWishart_values.push_back(mat);
		mat << 5.568622785229445, 8.063476382282362, 8.063476382282362, 14.617361420015273;
		inverseWishart_values.push_back(mat);
		mat << 18.602985103879114, -20.927024212576757, -20.927024212576757, 103.288509439176;
		inverseWishart_values.push_back(mat);
		mat << 31.967465120394188, -29.918059177204174, -29.918059177204174, 39.775957258693055;
		inverseWishart_values.push_back(mat);
		mat << 6.472471643069541, 7.982212242727661, 7.982212242727661, 15.183678658618968;
		inverseWishart_values.push_back(mat);
		mat << 33.90240388769968, -26.14422002740075, -26.14422002740075, 81.01007265720334;
		inverseWishart_values.push_back(mat);
		mat << 90.28933056074167, -109.57114987958012, -109.57114987958012, 154.73788781843925;
		inverseWishart_values.push_back(mat);
		mat << 3.615691611788429, 5.346253245914205, 5.346253245914205, 13.15814987559515;
		inverseWishart_values.push_back(mat);
		mat << 78.59453028428113, -57.01302037824069, -57.01302037824069, 119.45026718017952;
		inverseWishart_values.push_back(mat);
		mat << 74.47527326293206, -47.854810501155924, -47.854810501155924, 51.53432828416244;
		inverseWishart_values.push_back(mat);
		mat << 8.342987326203637, 8.907674823772057, 8.907674823772057, 11.052644414739712;
		inverseWishart_values.push_back(mat);
		mat << 30.775912283704844, -9.147745703117854, -9.147745703117854, 103.36256664175718;
		inverseWishart_values.push_back(mat);
		mat << 50.225264024395365, -45.85569277332643, -45.85569277332643, 66.49573379884538;
		inverseWishart_values.push_back(mat);
		mat << 6.835194118693804, 5.70172867450284, 5.70172867450284, 5.895664522472151;
		inverseWishart_values.push_back(mat);
		mat << 55.79148907786227, -47.632017739087374, -47.632017739087374, 87.32907517797219;
		inverseWishart_values.push_back(mat);
		mat << 56.27626208662955, -32.213348232949095, -32.213348232949095, 50.138822570685655;
		inverseWishart_values.push_back(mat);
		mat << 6.244253996520094, 5.853056013154145, 5.853056013154145, 7.247433212710751;
		inverseWishart_values.push_back(mat);
		mat << 31.717824571746625, -24.984563888501114, -24.984563888501114, 93.95687492506912;
		inverseWishart_values.push_back(mat);
		mat << 41.34669511528555, -31.115128952713857, -31.115128952713857, 41.79536024290296;
		inverseWishart_values.push_back(mat);
		mat << 6.920640561135847, 7.645432797069932, 7.645432797069932, 10.360103321013398;
		inverseWishart_values.push_back(mat);
		mat << 45.07021130384733, -12.189328162083525, -12.189328162083525, 51.45729419727372;
		inverseWishart_values.push_back(mat);
		mat << 63.5603972389988, -23.90548123504042, -23.90548123504042, 28.34264754936106;
		inverseWishart_values.push_back(mat);
		mat << 14.470439768953167, 18.071317914813747, 18.071317914813747, 26.25863720622487;
		inverseWishart_values.push_back(mat);
		mat << 11.837084770295522, 16.115017136296146, 16.115017136296146, 24.998027299505928;
		inverseWishart_values.push_back(mat);
		mat << 5.963836870098042, 6.016511669393422, 6.016511669393422, 8.087425382037688;
		inverseWishart_values.push_back(mat);
		mat << 150.79881422647924, -90.36173793748446, -90.36173793748446, 79.13499524586506;
		inverseWishart_values.push_back(mat);
		mat << 96.03284576274164, -74.85324772793999, -74.85324772793999, 76.43773580006773;
		inverseWishart_values.push_back(mat);
		prior->set_inverseWishart_values(inverseWishart_values);

		std::vector<double*> multinormal_values;
		multinormal_values.push_back(get_double_array(1.3035072131231256, 0.5243687544865521));
		multinormal_values.push_back(get_double_array(3.7609933157180557, -8.17805728016086));
		multinormal_values.push_back(get_double_array(1.5464477490758948, 5.7519807162781404));
		multinormal_values.push_back(get_double_array(0.8600313818216687, -0.09149834067039508));
		multinormal_values.push_back(get_double_array(3.0123143796106255, -2.6100802392281883));
		multinormal_values.push_back(get_double_array(-0.9177448328485478, 5.9515421569508655));
		multinormal_values.push_back(get_double_array(2.1576756373803443, -0.9178539583059258));
		multinormal_values.push_back(get_double_array(1.4056345018723135, -3.9561496232449294));
		multinormal_values.push_back(get_double_array(2.6364999309097064, 5.548979849591791));
		multinormal_values.push_back(get_double_array(3.0727214339310236, -0.23952329863294053));
		multinormal_values.push_back(get_double_array(-2.838373315215076, -3.764839661104511));
		multinormal_values.push_back(get_double_array(6.23791380652227, 10.65907047725142));
		multinormal_values.push_back(get_double_array(0.24368835759413265, 2.6145287552701313));
		multinormal_values.push_back(get_double_array(4.9270646898576125, -5.147729868940243));
		multinormal_values.push_back(get_double_array(6.129358588123798, 10.178997884039973));
		multinormal_values.push_back(get_double_array(0.9962747060187995, -0.7641292969095067));
		multinormal_values.push_back(get_double_array(-0.4309813089464156, -4.836644053003696));
		multinormal_values.push_back(get_double_array(4.107888999098682, 6.691385809616118));
		multinormal_values.push_back(get_double_array(1.1467334535474, -0.7790948502718439));
		multinormal_values.push_back(get_double_array(-1.822705501487424, -3.9738301507014344));
		multinormal_values.push_back(get_double_array(3.6372092105405467, 7.5684580222702955));
		multinormal_values.push_back(get_double_array(3.592254668853233, 0.9735141386718078));
		multinormal_values.push_back(get_double_array(1.6897480417856328, -5.231629573765232));
		multinormal_values.push_back(get_double_array(3.5999516879523608, 6.1883981044582015));
		multinormal_values.push_back(get_double_array(0.818633696359238, 3.0994063117443202));
		multinormal_values.push_back(get_double_array(3.6795296670649096, -7.0528870748644685));
		multinormal_values.push_back(get_double_array(5.142073707328128, 7.453130830105526));
		multinormal_values.push_back(get_double_array(1.6745425883772163, 0.19054061241717257));
		multinormal_values.push_back(get_double_array(3.2562015679228815, -5.696763497484621));
		multinormal_values.push_back(get_double_array(2.5417075192942646, 5.880339949226277));
		multinormal_values.push_back(get_double_array(0.8307276280923319, -1.8196937537678808));
		multinormal_values.push_back(get_double_array(6.680225838874014, -10.158725986437823));
		multinormal_values.push_back(get_double_array(5.223541971286749, 9.556906314700544));
		multinormal_values.push_back(get_double_array(4.074897229816364, -5.154206939509515));
		multinormal_values.push_back(get_double_array(2.6290452409623186, -6.925547046817949));
		multinormal_values.push_back(get_double_array(2.2207669576031166, 5.744769770160479));
		multinormal_values.push_back(get_double_array(3.7910687597094896, -0.22166894445465102));
		multinormal_values.push_back(get_double_array(1.691928530811514, -6.487974755077392));
		multinormal_values.push_back(get_double_array(2.9833081759890034, 5.652085989142228));
		multinormal_values.push_back(get_double_array(7.504544840142425, -0.11814099865233141));
		multinormal_values.push_back(get_double_array(4.258572558247976, -7.2650522177313945));
		multinormal_values.push_back(get_double_array(4.574484901121846, 8.87947443318769));
		multinormal_values.push_back(get_double_array(5.437762175586767, -3.26000202557786));
		multinormal_values.push_back(get_double_array(5.206641366283937, -9.213219789390283));
		multinormal_values.push_back(get_double_array(1.345190617627447, 4.566724517283887));
		multinormal_values.push_back(get_double_array(6.621546950906058, 12.006047941478377));
		multinormal_values.push_back(get_double_array(3.5842980581569184, 7.624475555638508));
		multinormal_values.push_back(get_double_array(7.415255287091461, -12.537681691017395));
		multinormal_values.push_back(get_double_array(5.6152664362115186, -10.142473005342296));
		prior->set_multinormal_values(multinormal_values);


		for (int i = 1; i <= 15; i++)
		{
			if (i == 3)
			{
				//left
				niw_sufficient_statistics* ss = dynamic_cast<niw_sufficient_statistics*>(cluster_params->cluster_params_l->suff_statistics.get());

				ss->N = 7;
				ss->points_sum << 30.267637, -26.16328;
				ss->S << 562.07477, -328.94397, -328.94397, 1025.3411;

				niw_hyperparams* nh = dynamic_cast<niw_hyperparams*>(cluster_params->cluster_params_l->posterior_hyperparams.get());

				nh->k = 8.0;
				nh->m << 3.7834547, -3.27041;
				nh->v = 12.0;
				nh->psi << 37.71321, -19.163033, -19.163033, 78.73137;

				//right
				ss = dynamic_cast<niw_sufficient_statistics*>(cluster_params->cluster_params_r->suff_statistics.get());
				ss->N = 3;
				ss->points_sum << 4.0300894, 17.54369;
				ss->S << 36.099518, 58.01129, 58.01129, 144.20627;

				nh = dynamic_cast<niw_hyperparams*>(cluster_params->cluster_params_r->posterior_hyperparams.get());
				nh->k = 4.0;
				nh->m << 1.0075223, 4.3859224;
				nh->v = 8.0;
				nh->psi << 4.629889, 5.041954, 5.041954, 9.032626;

			}
			else if (i == 4)
			{
				//left
				niw_sufficient_statistics* ss = dynamic_cast<niw_sufficient_statistics*>(cluster_params->cluster_params_l->suff_statistics.get());

				ss->N = 6;
				ss->points_sum << 15.408037, -44.35743;
				ss->S << 479.19385, -476.44736, -476.44736, 803.72107;

				niw_hyperparams* nh = dynamic_cast<niw_hyperparams*>(cluster_params->cluster_params_l->posterior_hyperparams.get());
				nh->k = 7.0;
				nh->m << 2.2011483, -6.336776;
				nh->v = 11.0;
				nh->psi << 40.934406, -34.43728, -34.43728, 47.967087;

				//right
				ss = dynamic_cast<niw_sufficient_statistics*>(cluster_params->cluster_params_r->suff_statistics.get());

				ss->N = 4;
				ss->points_sum << 18.889688, 35.73784;
				ss->S << 118.98046, 205.5147, 205.5147, 365.82626;

				nh = dynamic_cast<niw_hyperparams*>(cluster_params->cluster_params_r->posterior_hyperparams.get());

				nh->k = 5.0;
				nh->m << 3.7779377, 7.1475677;
				nh->v = 9.0;
				nh->psi << 5.846266, 7.833262, 7.833262, 12.820848;

			}
			object.sample_cluster_params(cluster_params, 10, false);
		}

		EXPECT_NEAR(-103.156006f, cluster_params->logsublikelihood_hist[0].second, 0.0001f);
		EXPECT_NEAR(-103.156006f, cluster_params->logsublikelihood_hist[1].second, 0.0001f);
		EXPECT_NEAR(-92.4774f, cluster_params->logsublikelihood_hist[2].second, 0.0001f);
		EXPECT_NEAR(-82.60051f, cluster_params->logsublikelihood_hist[3].second, 0.0001f);
	}	
}