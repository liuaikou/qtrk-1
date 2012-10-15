
#pragma once
#include <cstdlib>
#include <cmath>

template<typename T>
T rand_uniform()
{
	return rand() / (T)RAND_MAX;
}


template<typename T>
T rand_normal()
{
	T U0 = rand_uniform<T>();
	T U1 = rand_uniform<T>();

	// Box-Muller transform
	return sqrt( -2 * log(U0) ) * cos(2*(T)3.141593 * U1);
}

template<typename T>
int rand_poisson(T lambda)
{
	if (lambda > 10) {
		T v = rand_normal<T>();
		return (int)max(0,((T)0.5 + lambda + v*sqrt(lambda)));
	}
	else {
		T L = exp(-lambda);
		int k = 0;
		T p = 1;
		do {
			k++;
			T u = rand_uniform<T>();
			p = p * u;
		} while (p > L && k<200);
	
		// copy back
		return k-1;
	}
}


