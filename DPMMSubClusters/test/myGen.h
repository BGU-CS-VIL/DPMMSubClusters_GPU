#pragma once

#include <random>

class myGen : public std::mt19937
{
public:
	_NODISCARD result_type operator()()
	{
		return 2;// 0.5;
	}

	_NODISCARD static constexpr result_type(min)()
	{
		return 1;
	}

	/*double max()
	{
		return m_max;
	}*/

	_NODISCARD static constexpr result_type(max)()
	{
		return 2;
	}
};