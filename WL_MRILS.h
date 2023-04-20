// Copyright (C) 2022  Marcelo R. H. Maia <mmaia@ic.uff.br, marcelo.h.maia@ibge.gov.br>


#include <set>
#include <unordered_set>
#include <vector>

#include "WL_Instance.h"
#include "WL_Solution.h"

#define MY_EPSILON 0.00001 // Precision parameter, used to avoid numerical instabilities

// MineReduce-based Multi-Start ILS solver for the WLP
class WL_MRILS
{
public:
	WL_MRILS(WL_Instance& i, unsigned timeout, unsigned seed, unsigned elite_max_size, double stabi_param, double min_sup, unsigned n_patterns, bool random_opening, unsigned ils_maxiter, double ils_accept);
	void Run();
	WL_Solution* Best() const { return best; }
	double TimeBest() const { return time_best; }
private:
	WL_Instance& in;
	WL_Solution* best;
	double time_best;
	unsigned timeout, seed, elite_max_size, max_nu_iter, n_patterns, ils_maxiter;
	double min_sup, ils_accept, stabi_param;
	bool random_opening;
	set<WL_Solution,bool(*)(WL_Solution,WL_Solution)> elite;
	vector<vector<Supply>> patterns;
	vector<WL_Instance> reduced_instances;
	WL_Solution* InitialSolution();
	WL_Solution* InitialSolutionGreedyOpening();
	WL_Solution* InitialSolutionRandomOpening();
	void LocalSearch(WL_Solution* sol);
	WL_Solution* IteratedLocalSearch(WL_Solution* sol);
	unsigned Perturbation(WL_Solution* sol, unordered_set<unsigned>* invalid_warehouses, unordered_set<unsigned>* closing_forbidden, unordered_set<unsigned>* opening_forbidden);
	void MineElite();
	WL_Instance ReducedInstance(unsigned p);
};