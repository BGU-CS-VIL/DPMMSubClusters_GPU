#pragma once

#include "priors/niw.h"

class myNiw : public niw
{
	public:
		myNiw()
		{
			inverseWishartIter = 0;
			multinormalIter = 0;
		}
		void set_inverseWishart_values(std::vector<MatrixXd> &values)
		{
			inverseWishart_values = values;
		}
		void set_multinormal_values(std::vector<double*> &values)
		{
			multinormal_values = values;
		}

protected:
	virtual MatrixXd inverseWishart(const MatrixXd& sigma, double v)
	{
		++inverseWishartIter;
		if (inverseWishart_values.size() < inverseWishartIter)
		{
			MatrixXd result(2, 2);
			result << 8.402266651681197, -0.09345429531760577, -0.09345429531760577, 12.197528108693033;
			return result;
		}
		else
		{
			return inverseWishart_values[inverseWishartIter - 1];
		}
	}

	virtual double* multinormal_sample(int n, double mu[], double r[])
	{
		++multinormalIter;
		if (inverseWishart_values.size() < multinormalIter)
		{
			double *result = new double(2);
			result[0] = -2.6763540674250113;
			result[1] = -6.373344698686574;

			return result;
		}
		else
		{
			return multinormal_values[multinormalIter - 1];
		}

	}

private:
	int inverseWishartIter;
	int multinormalIter;
	std::vector<MatrixXd> inverseWishart_values;
	std::vector<double*> multinormal_values;
};
