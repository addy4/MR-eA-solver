// Copyright (C) 2022  Marcelo R. H. Maia <mmaia@ic.uff.br, marcelo.h.maia@ibge.gov.br>

#include <algorithm>
#include <climits>
#include <iostream>
#include <queue>
#include "fpmax.h"

#include "WL_MRILS.h"

extern "C"
{
#include "pcea-solution.h"
}

using namespace std;

// Comparator for ordering warehouses by their fixed-cost to capacity ratio
struct WarehouseComparator
{
	WarehouseComparator(const WL_Instance &in) : in(in){};
	bool operator()(unsigned i, unsigned j)
	{
		return ((double)in.FixedCost(i)) / in.Capacity(i) < ((double)in.FixedCost(j)) / in.Capacity(j);
	}

	const WL_Instance &in;
};

// Comparator for ordering solutions by cost
bool CompareSolutions(WL_Solution sol1, WL_Solution sol2)
{
	return sol1.Cost() < sol2.Cost() - MY_EPSILON;
}

// Move structure
// If store `s2` is out of range, then type I: supply to store `s1` by warehouse `w1` is reassigned to warehouse `w2`
// 			(the quantity reassigned is the max between the quantity assigned to `w1` and the residual capacity of `w2`)
// Otherwise, type II: supply to store `s1` by warehouse `w1` is swaped with supply to store `s2` by warehouse `w2` - i.e. {(w1, s1, q1), (w2, s2, q2)} -> {(w1, s2, q2), (w2, s1, q1)}
struct Move
{
	unsigned s1, s2, w1, w2;
	double improvement;
};

// Comparator for ordering moves by improvement
struct MoveComparator
{
	bool operator()(Move m1, Move m2)
	{
		return m1.improvement < m2.improvement;
	}
};

WL_MRILS::WL_MRILS(WL_Instance &my_in, unsigned timeout, unsigned seed, unsigned elite_max_size, double stabi_param,
				   double min_sup, unsigned n_patterns, bool random_opening, unsigned ils_maxiter, double ils_accept)
	: in(my_in), timeout(timeout), seed(seed), elite_max_size(elite_max_size), n_patterns(n_patterns),
	  ils_maxiter(ils_maxiter), min_sup(min_sup), ils_accept(ils_accept), stabi_param(stabi_param), random_opening(random_opening),
	  elite(CompareSolutions)
{
}

void WL_MRILS::Run(int pcea_routine)
{

	best = NULL;

	unsigned i = 0;
	unsigned nu_iter = 0;
	unsigned max_nu_iter = 0;
	bool elite_updated = false;
	unsigned p = 0;

	while (clock() / CLOCKS_PER_SEC < timeout)
	{
		i++;

		cout << "iteration " << i << endl;

		if (elite_max_size && elite_updated && (nu_iter > max_nu_iter || (elite.size() == elite_max_size && patterns.empty() && clock() / CLOCKS_PER_SEC > timeout / 2)))
		{
			cout << "mining elite..." << flush;
			MineElite();
			reduced_instances.clear();
			elite_updated = false;
			p = 0;
			cout << " finished" << endl;
		}

		WL_Solution *sol;
		if (patterns.empty())
		{
			cout << "generating initial solution..." << flush;
			sol = InitialSolution();
			cout << " finished" << endl;
		}
		else
		{

			if (pcea_routine)
			{
				for (unsigned j = 0; j < patterns.size(); j++)
				{
					for (unsigned t = 0; t < patterns[j].size(); t++)
					{
						cout << "{" << patterns[j][t].s << ", " << patterns[j][t].w << ", " << patterns[j][t].q << "}" << endl;
					}

					WL_Instance original_instance = in;
					in = ReducedInstance(p);

					in.getFixedCost();

					cout << endl;
					cout << endl;
				}
				exit(1);
			}

			WL_Instance original_instance = in;
			in = ReducedInstance(p);

			cout << "generating initial solution (reduced)..." << flush;
			WL_Solution *reduced_sol = InitialSolution();
			cout << " finished" << endl;
			cout << "local search (reduced)..." << flush;
			reduced_sol = IteratedLocalSearch(reduced_sol);
			cout << " finished" << endl;

			in = original_instance;
			sol = new WL_Solution(in);
			for (unsigned w = 0; w < in.Warehouses(); w++)
				for (auto it = reduced_sol->supplied_stores[w].begin(); it != reduced_sol->supplied_stores[w].end(); ++it)
				{
					unsigned s = *it;
					sol->Assign(s, w, reduced_sol->Supply(s, w));
				}
			for (unsigned j = 0; j < patterns[p].size(); j++)
				sol->Assign(patterns[p][j].s, patterns[p][j].w, patterns[p][j].q);

			delete reduced_sol;
			p = (p + 1) % patterns.size();
		}

		cout << "local search..." << flush;
		sol = IteratedLocalSearch(sol);
		cout << " finished" << endl;

		if (elite_max_size)
		{
			nu_iter++;
			unsigned old_elite_size = elite.size();
			elite.insert(*sol);
			if (elite.size() > elite_max_size)
			{
				set<WL_Solution>::iterator it = --elite.end();
				if (it->Cost() - MY_EPSILON > sol->Cost())
				{
					nu_iter = 0;
					elite_updated = true;
				}
				elite.erase(it);
			}
			else if (elite.size() > old_elite_size)
			{
				nu_iter = 0;
				elite_updated = true;
			}
		}

		if (best == NULL || sol->Cost() < best->Cost() - MY_EPSILON)
		{
			time_best = (double)clock() / CLOCKS_PER_SEC;

			if (best != NULL)
				delete best;

			best = sol->Copy();
		}

		delete sol;

		unsigned est_n_iter = min(1000, (int)(timeout / (((double)clock() / CLOCKS_PER_SEC) / i)));
		max_nu_iter = stabi_param * est_n_iter;
	}
}

// Generates an initial solution
WL_Solution *WL_MRILS::InitialSolution()
{
	if (random_opening)
		return InitialSolutionRandomOpening();

	return InitialSolutionGreedyOpening();
}

// Generates an initial solution with greedy selection of warehouses to open
WL_Solution *WL_MRILS::InitialSolutionGreedyOpening()
{
	WL_Solution *sol;
	bool feasible = false;

	while (!feasible)
	{
		sol = new WL_Solution(in);
		feasible = true;

		vector<unsigned> warehouses(in.Warehouses());
		for (unsigned w = 0; w < in.Warehouses(); w++)
			warehouses[w] = w;

		sort(warehouses.begin(), warehouses.end(), WarehouseComparator(in));

		unsigned total_demand = 0;
		for (unsigned s = 0; s < in.Stores(); s++)
			total_demand += in.AmountOfGoods(s);

		unsigned last_open = 0;
		unsigned total_capacity = in.Capacity(warehouses[0]);
		for (unsigned w = 1; total_capacity < total_demand; w++)
		{
			last_open = w;
			total_capacity += in.Capacity(warehouses[w]);
		}

		for (unsigned w = 0; w <= last_open; w++)
		{
			if (sol->ResidualCapacity(warehouses[w]))
			{
				unsigned s = rand() % in.Stores();
				unsigned trials = 0;
				while (!sol->ResidualAmount(s) || sol->Incompatibilities(warehouses[w], s))
				{
					if (++trials > in.Stores())
						break;

					s = rand() % in.Stores();
				}

				if (trials <= in.Stores())
					sol->Assign(s, warehouses[w], min(sol->ResidualAmount(s), in.Capacity(warehouses[w])));
			}
		}

		for (unsigned s = 0; feasible && s < in.Stores(); s++)
		{
			while (sol->ResidualAmount(s))
			{
				unsigned best_w = in.Warehouses();
				for (unsigned w = 0; w <= last_open; w++)
					if (sol->ResidualCapacity(warehouses[w]) && !sol->Incompatibilities(warehouses[w], s) && (best_w == in.Warehouses() || in.SupplyCost(s, warehouses[w]) < in.SupplyCost(s, best_w)))
						best_w = warehouses[w];

				if (best_w == in.Warehouses())
				{
					unsigned next = last_open + 1;
					while (next < in.Warehouses() && (!sol->ResidualCapacity(warehouses[next]) || sol->Incompatibilities(warehouses[next], s)))
						next++;

					if (next < in.Warehouses())
					{
						unsigned next_w = warehouses[next];
						last_open++;
						for (unsigned i = next; i > last_open; i--)
							warehouses[i] = warehouses[i - 1];
						warehouses[last_open] = next_w;
						best_w = next_w;
					}
					else
					{
						feasible = false;
						delete sol;
						break;
					}
				}

				sol->Assign(s, best_w, min(sol->ResidualAmount(s), sol->ResidualCapacity(best_w)));
			}
		}
	}

	return sol;
}

// Generates an initial solution with random (roulette) selection of warehouses to open
WL_Solution *WL_MRILS::InitialSolutionRandomOpening()
{
	WL_Solution *sol;
	bool feasible = false;

	while (!feasible)
	{
		sol = new WL_Solution(in);
		feasible = true;

		vector<unsigned> warehouses(in.Warehouses());
		double relative_cost_sum = 0;
		for (unsigned w = 0; w < in.Warehouses(); w++)
		{
			warehouses[w] = w;
			relative_cost_sum += in.FixedCost(w) ? (double)in.Capacity(w) / in.FixedCost(w) : (double)in.Capacity(w);
		}

		unsigned total_demand = 0;
		for (unsigned s = 0; s < in.Stores(); s++)
			total_demand += in.AmountOfGoods(s);

		int last_open = -1;
		unsigned total_capacity = 0;
		while (total_capacity < total_demand)
		{
			double random = (double)rand() / RAND_MAX;
			double cumulative_prob = 0;
			for (unsigned w = last_open + 1; w < in.Warehouses(); w++)
			{
				double selection_prob = (in.FixedCost(warehouses[w]) ? (double)in.Capacity(warehouses[w]) / in.FixedCost(warehouses[w]) : (double)in.Capacity(warehouses[w])) / relative_cost_sum;
				if (random <= cumulative_prob + selection_prob)
				{
					unsigned temp = warehouses[++last_open];
					warehouses[last_open] = warehouses[w];
					warehouses[w] = temp;
					total_capacity += in.Capacity(warehouses[last_open]);
					relative_cost_sum -= in.FixedCost(warehouses[last_open]) ? (double)in.Capacity(warehouses[last_open]) / in.FixedCost(warehouses[last_open]) : (double)in.Capacity(warehouses[last_open]);
					break;
				}
				cumulative_prob += selection_prob;
			}
		}

		for (int w = 0; w <= last_open; w++)
		{
			if (sol->ResidualCapacity(warehouses[w]))
			{
				unsigned s = rand() % in.Stores();
				unsigned trials = 0;
				while (!sol->ResidualAmount(s) || sol->Incompatibilities(warehouses[w], s))
				{
					if (++trials > in.Stores())
						break;

					s = rand() % in.Stores();
				}

				if (trials <= in.Stores())
					sol->Assign(s, warehouses[w], min(sol->ResidualAmount(s), in.Capacity(warehouses[w])));
			}
		}

		for (unsigned s = 0; feasible && s < in.Stores(); s++)
		{
			while (sol->ResidualAmount(s))
			{
				unsigned best_w = in.Warehouses();
				for (int w = 0; w <= last_open; w++)
					if (sol->ResidualCapacity(warehouses[w]) && !sol->Incompatibilities(warehouses[w], s) && (best_w == in.Warehouses() || in.SupplyCost(s, warehouses[w]) < in.SupplyCost(s, best_w)))
						best_w = warehouses[w];

				if (best_w == in.Warehouses())
				{
					if (last_open < (int)in.Warehouses() - 1)
						while (best_w == in.Warehouses())
						{
							double random = (double)rand() / RAND_MAX;
							double cumulative_prob = 0;
							for (unsigned w = last_open + 1; w < in.Warehouses(); w++)
							{
								double selection_prob = (in.FixedCost(warehouses[w]) ? (double)in.Capacity(warehouses[w]) / in.FixedCost(warehouses[w]) : (double)in.Capacity(warehouses[w])) / relative_cost_sum;
								if (random <= cumulative_prob + selection_prob)
								{
									if (sol->ResidualCapacity(warehouses[w]) && !sol->Incompatibilities(warehouses[w], s))
									{
										unsigned temp = warehouses[++last_open];
										warehouses[last_open] = warehouses[w];
										warehouses[w] = temp;
										total_capacity += in.Capacity(warehouses[last_open]);
										relative_cost_sum -= in.FixedCost(warehouses[last_open]) ? (double)in.Capacity(warehouses[last_open]) / in.FixedCost(warehouses[last_open]) : (double)in.Capacity(warehouses[last_open]);
										best_w = warehouses[last_open];
									}
									break;
								}
								cumulative_prob += selection_prob;
							}
						}
					else
					{
						feasible = false;
						delete sol;
						break;
					}
				}

				sol->Assign(s, best_w, min(sol->ResidualAmount(s), sol->ResidualCapacity(best_w)));
			}
		}
	}

	return sol;
}

// Local search using a priority queue of improving moves and multi improvement strategy
void WL_MRILS::LocalSearch(WL_Solution *sol)
{
	unordered_set<unsigned> invalid_warehouses;

	for (unsigned w = 0; w < in.Warehouses(); w++)
		invalid_warehouses.insert(w);

	priority_queue<Move, vector<Move>, MoveComparator> moves;

	while (clock() / CLOCKS_PER_SEC < timeout)
	{
		// (Re)compute moves for invalid warehouses

		for (auto it = invalid_warehouses.begin(); it != invalid_warehouses.end(); ++it)
		{
			unsigned w1 = *it;
			if (sol->Load(w1))
				for (auto it2 = sol->supplied_stores[w1].begin(); it2 != sol->supplied_stores[w1].end(); ++it2)
				{
					unsigned s1 = *it2;
					for (unsigned w2 = 0; w2 < in.Warehouses(); w2++)
						if (w1 != w2)
						{
							// Neighborhood 1: solutions that can be obtained from `sol` by relocating the allowed maximum
							// quantity of goods supplied to a store (s1) from one warehouse (w1) to another (w2)
							if (!sol->Incompatibilities(w2, s1) && sol->ResidualCapacity(w2))
							{
								unsigned q = min(sol->Supply(s1, w1), sol->ResidualCapacity(w2));
								double improvement = (in.SupplyCost(s1, w1) - in.SupplyCost(s1, w2)) * q;
								if (!sol->Load(w2))
									improvement -= in.FixedCost(w2);
								if (q == sol->Load(w1))
									improvement += in.FixedCost(w1);

								if (improvement > MY_EPSILON)
									moves.push({s1, in.Stores(), w1, w2, improvement});
							}

							// Neighborhood 2: solutions that can be obtained from `sol` by exchanging one store (s1)
							// from one warehouse (w1) with another store (s2) from another warehouse (w2)
							if (sol->Incompatibilities(w2, s1) <= 1)
								for (auto it3 = sol->supplied_stores[w2].begin(); it3 != sol->supplied_stores[w2].end(); ++it3)
								{
									unsigned s2 = *it3;
									if (s1 != s2 && ((!sol->Incompatibilities(w1, s2) && !sol->Incompatibilities(w2, s1)) || (sol->Incompatibilities(w1, s2) == 1 && in.Incompatible(s1, s2))) && sol->Supply(s1, w1) <= sol->ResidualCapacity(w2) + sol->Supply(s2, w2) && sol->Supply(s2, w2) <= sol->ResidualCapacity(w1) + sol->Supply(s1, w1))
									{
										double improvement = (in.SupplyCost(s1, w1) - in.SupplyCost(s1, w2)) * sol->Supply(s1, w1) + (in.SupplyCost(s2, w2) - in.SupplyCost(s2, w1)) * sol->Supply(s2, w2);

										if (improvement > MY_EPSILON)
											moves.push({s1, s2, w1, w2, improvement});
									}
								}
						}
				}
		}

		for (unsigned w1 = 0; w1 < in.Warehouses(); w1++)
			if (sol->Load(w1))
				for (auto it = sol->supplied_stores[w1].begin(); it != sol->supplied_stores[w1].end(); ++it)
				{
					unsigned s1 = *it;
					for (auto it2 = invalid_warehouses.begin(); it2 != invalid_warehouses.end(); ++it2)
					{
						unsigned w2 = *it2;
						if (w1 != w2)
						{
							// Neighborhood 1: solutions that can be obtained from `sol` by relocating the allowed maximum
							// quantity of goods supplied to a store (s1) from one warehouse (w1) to another (w2)
							if (!sol->Incompatibilities(w2, s1) && sol->ResidualCapacity(w2))
							{
								unsigned q = min(sol->Supply(s1, w1), sol->ResidualCapacity(w2));
								double improvement = (in.SupplyCost(s1, w1) - in.SupplyCost(s1, w2)) * q;
								if (!sol->Load(w2))
									improvement -= in.FixedCost(w2);
								if (q == sol->Load(w1))
									improvement += in.FixedCost(w1);

								if (improvement > MY_EPSILON)
									moves.push({s1, in.Stores(), w1, w2, improvement});
							}

							// Neighborhood 2: solutions that can be obtained from `sol` by exchanging one store (s1)
							// from one warehouse (w1) with another store (s2) from another warehouse (w2)
							if (sol->Incompatibilities(w2, s1) <= 1)
								for (auto it3 = sol->supplied_stores[w2].begin(); it3 != sol->supplied_stores[w2].end(); ++it3)
								{
									unsigned s2 = *it3;
									if (s1 != s2 && ((!sol->Incompatibilities(w1, s2) && !sol->Incompatibilities(w2, s1)) || (sol->Incompatibilities(w1, s2) == 1 && in.Incompatible(s1, s2))) && sol->Supply(s1, w1) <= sol->ResidualCapacity(w2) + sol->Supply(s2, w2) && sol->Supply(s2, w2) <= sol->ResidualCapacity(w1) + sol->Supply(s1, w1))
									{
										double improvement = (in.SupplyCost(s1, w1) - in.SupplyCost(s1, w2)) * sol->Supply(s1, w1) + (in.SupplyCost(s2, w2) - in.SupplyCost(s2, w1)) * sol->Supply(s2, w2);

										if (improvement > MY_EPSILON)
											moves.push({s1, s2, w1, w2, improvement});
									}
								}
						}
					}
				}

		if (moves.empty())
			break;

		invalid_warehouses.clear();

		while (!moves.empty() && clock() / CLOCKS_PER_SEC < timeout)
		{
			Move move = moves.top();
			moves.pop();

			if (invalid_warehouses.find(move.w1) != invalid_warehouses.end() || invalid_warehouses.find(move.w2) != invalid_warehouses.end())
				continue;

			if (move.s2 == in.Stores())
			{
				unsigned q = min(sol->Supply(move.s1, move.w1), sol->ResidualCapacity(move.w2));
				sol->RevokeAssignment(move.s1, move.w1, q);
				sol->Assign(move.s1, move.w2, q);
			}
			else
			{
				unsigned q = sol->Supply(move.s1, move.w1);
				sol->RevokeAssignment(move.s1, move.w1, q);
				sol->Assign(move.s1, move.w2, q);

				q = sol->Supply(move.s2, move.w2);
				sol->RevokeAssignment(move.s2, move.w2, q);
				sol->Assign(move.s2, move.w1, q);
			}

			// Invalidate warehouses affected by last move
			invalid_warehouses.insert(move.w1);
			invalid_warehouses.insert(move.w2);
		}
	}
}

// ILS using a priority queue of improving moves and multi improvement strategy
WL_Solution *WL_MRILS::IteratedLocalSearch(WL_Solution *sol)
{
	if (ils_maxiter == 1)
	{
		LocalSearch(sol);
		return sol;
	}

	WL_Solution *best_sol = sol->Copy();
	WL_Solution *working_sol = NULL;

	unordered_set<unsigned> invalid_warehouses, closing_forbidden, opening_forbidden;

	for (unsigned w = 0; w < in.Warehouses(); w++)
		invalid_warehouses.insert(w);

	priority_queue<Move, vector<Move>, MoveComparator> moves;

	for (unsigned i = 0; clock() / CLOCKS_PER_SEC < timeout && i < ils_maxiter; i++)
	{
		if (i > 0)
		{
			if (sol->Cost() + MY_EPSILON < ils_accept * best_sol->Cost())
			{
				if (working_sol)
					delete working_sol;
				working_sol = sol->Copy();
			}
			else
			{
				delete sol;
				sol = working_sol->Copy();
			}

			unsigned perturbation = 0;
			for (unsigned trials = 0; !perturbation && trials < 5; trials++)
				perturbation = Perturbation(sol, &invalid_warehouses, &closing_forbidden, &opening_forbidden);

			if (!perturbation)
				break;
		}

		while (clock() / CLOCKS_PER_SEC < timeout)
		{
			// (Re)compute moves for invalid warehouses

			for (auto it = invalid_warehouses.begin(); it != invalid_warehouses.end(); ++it)
			{
				unsigned w1 = *it;
				if (sol->Load(w1))
					for (auto it2 = sol->supplied_stores[w1].begin(); it2 != sol->supplied_stores[w1].end(); ++it2)
					{
						unsigned s1 = *it2;
						for (unsigned w2 = 0; w2 < in.Warehouses(); w2++)
							if (w1 != w2 && opening_forbidden.find(w2) == opening_forbidden.end())
							{
								// Neighborhood 1: solutions that can be obtained from `sol` by relocating the allowed maximum
								// quantity of goods supplied to a store (s1) from one warehouse (w1) to another (w2)
								if (!sol->Incompatibilities(w2, s1) && sol->ResidualCapacity(w2))
								{
									unsigned q = min(sol->Supply(s1, w1), sol->ResidualCapacity(w2));
									double improvement = (in.SupplyCost(s1, w1) - in.SupplyCost(s1, w2)) * q;
									if (!sol->Load(w2))
										improvement -= in.FixedCost(w2);
									if (q == sol->Load(w1) && closing_forbidden.find(w1) == closing_forbidden.end())
										improvement += in.FixedCost(w1);

									if (improvement > MY_EPSILON)
										moves.push({s1, in.Stores(), w1, w2, improvement});
								}

								// Neighborhood 2: solutions that can be obtained from `sol` by exchanging one store (s1)
								// from one warehouse (w1) with another store (s2) from another warehouse (w2)
								if (sol->Incompatibilities(w2, s1) <= 1)
									for (auto it3 = sol->supplied_stores[w2].begin(); it3 != sol->supplied_stores[w2].end(); ++it3)
									{
										unsigned s2 = *it3;
										if (s1 != s2 && ((!sol->Incompatibilities(w1, s2) && !sol->Incompatibilities(w2, s1)) || (sol->Incompatibilities(w1, s2) == 1 && in.Incompatible(s1, s2))) && sol->Supply(s1, w1) <= sol->ResidualCapacity(w2) + sol->Supply(s2, w2) && sol->Supply(s2, w2) <= sol->ResidualCapacity(w1) + sol->Supply(s1, w1))
										{
											double improvement = (in.SupplyCost(s1, w1) - in.SupplyCost(s1, w2)) * sol->Supply(s1, w1) + (in.SupplyCost(s2, w2) - in.SupplyCost(s2, w1)) * sol->Supply(s2, w2);

											if (improvement > MY_EPSILON)
												moves.push({s1, s2, w1, w2, improvement});
										}
									}
							}
					}
			}

			for (unsigned w1 = 0; w1 < in.Warehouses(); w1++)
				if (sol->Load(w1))
					for (auto it = sol->supplied_stores[w1].begin(); it != sol->supplied_stores[w1].end(); ++it)
					{
						unsigned s1 = *it;
						for (auto it2 = invalid_warehouses.begin(); it2 != invalid_warehouses.end(); ++it2)
						{
							unsigned w2 = *it2;
							if (w1 != w2 && opening_forbidden.find(w2) == opening_forbidden.end())
							{
								// Neighborhood 1: solutions that can be obtained from `sol` by relocating the allowed maximum
								// quantity of goods supplied to a store (s1) from one warehouse (w1) to another (w2)
								if (!sol->Incompatibilities(w2, s1) && sol->ResidualCapacity(w2))
								{
									unsigned q = min(sol->Supply(s1, w1), sol->ResidualCapacity(w2));
									double improvement = (in.SupplyCost(s1, w1) - in.SupplyCost(s1, w2)) * q;
									if (!sol->Load(w2))
										improvement -= in.FixedCost(w2);
									if (q == sol->Load(w1) && closing_forbidden.find(w1) == closing_forbidden.end())
										improvement += in.FixedCost(w1);

									if (improvement > MY_EPSILON)
										moves.push({s1, in.Stores(), w1, w2, improvement});
								}

								// Neighborhood 2: solutions that can be obtained from `sol` by exchanging one store (s1)
								// from one warehouse (w1) with another store (s2) from another warehouse (w2)
								if (sol->Incompatibilities(w2, s1) <= 1)
									for (auto it3 = sol->supplied_stores[w2].begin(); it3 != sol->supplied_stores[w2].end(); ++it3)
									{
										unsigned s2 = *it3;
										if (s1 != s2 && ((!sol->Incompatibilities(w1, s2) && !sol->Incompatibilities(w2, s1)) || (sol->Incompatibilities(w1, s2) == 1 && in.Incompatible(s1, s2))) && sol->Supply(s1, w1) <= sol->ResidualCapacity(w2) + sol->Supply(s2, w2) && sol->Supply(s2, w2) <= sol->ResidualCapacity(w1) + sol->Supply(s1, w1))
										{
											double improvement = (in.SupplyCost(s1, w1) - in.SupplyCost(s1, w2)) * sol->Supply(s1, w1) + (in.SupplyCost(s2, w2) - in.SupplyCost(s2, w1)) * sol->Supply(s2, w2);

											if (improvement > MY_EPSILON)
												moves.push({s1, s2, w1, w2, improvement});
										}
									}
							}
						}
					}

			if (moves.empty())
				break;

			invalid_warehouses.clear();

			while (!moves.empty() && clock() / CLOCKS_PER_SEC < timeout)
			{
				Move move = moves.top();
				moves.pop();

				if (invalid_warehouses.find(move.w1) != invalid_warehouses.end() || invalid_warehouses.find(move.w2) != invalid_warehouses.end())
					continue;

				if (move.s2 == in.Stores())
				{
					unsigned q = min(sol->Supply(move.s1, move.w1), sol->ResidualCapacity(move.w2));
					sol->RevokeAssignment(move.s1, move.w1, q);
					sol->Assign(move.s1, move.w2, q);
				}
				else
				{
					unsigned q = sol->Supply(move.s1, move.w1);
					sol->RevokeAssignment(move.s1, move.w1, q);
					sol->Assign(move.s1, move.w2, q);

					q = sol->Supply(move.s2, move.w2);
					sol->RevokeAssignment(move.s2, move.w2, q);
					sol->Assign(move.s2, move.w1, q);
				}

				// Invalidate warehouses affected by last move
				invalid_warehouses.insert(move.w1);
				invalid_warehouses.insert(move.w2);
			}

			if (sol->Cost() < best_sol->Cost() - MY_EPSILON)
			{
				delete best_sol;
				best_sol = sol->Copy();
			}
		}
	}

	delete sol;
	if (working_sol)
		delete working_sol;

	return best_sol;
}

// Solution perturbation
unsigned WL_MRILS::Perturbation(WL_Solution *sol, unordered_set<unsigned> *invalid_warehouses, unordered_set<unsigned> *closing_forbidden, unordered_set<unsigned> *opening_forbidden)
{
	closing_forbidden->clear();
	opening_forbidden->clear();

	unsigned perturbation = 1 + rand() % 5;

	switch (perturbation)
	{
	case 1: // Perturbation 1 (close a warehouse)
	{
		vector<unsigned> candidates;
		for (unsigned w = 0; w < in.Warehouses(); w++)
			if (sol->supplied_stores[w].size() == 1 && in.FixedCost(w))
				candidates.push_back(w);

		if (candidates.empty())
			return 0;

		unsigned w1 = candidates[rand() % candidates.size()];
		unsigned s = *(sol->supplied_stores[w1].begin());

		sol->RevokeAssignment(s, w1, sol->Supply(s, w1));

		while (sol->ResidualAmount(s))
		{
			unsigned best_w = in.Warehouses();
			for (unsigned w2 = 0; w2 < in.Warehouses(); w2++)
				if ((sol->Load(w2) || !in.FixedCost(w2)) && sol->ResidualCapacity(w2) && !sol->Incompatibilities(w2, s) && (best_w == in.Warehouses() || in.SupplyCost(s, w2) < in.SupplyCost(s, best_w)))
					best_w = w2;

			if (best_w == in.Warehouses())
			{
				for (unsigned w2 = 0; w2 < in.Warehouses(); w2++)
					if (w2 != w1 && !sol->Load(w2) && in.FixedCost(w2) && sol->ResidualCapacity(w2) && (best_w == in.Warehouses() || in.SupplyCost(s, w2) < in.SupplyCost(s, best_w)))
						best_w = w2;
			}

			sol->Assign(s, best_w, min(sol->ResidualAmount(s), sol->ResidualCapacity(best_w)));

			invalid_warehouses->insert(best_w);
		}

		opening_forbidden->insert(w1);

		break;
	}
	case 2: // Perturbation 2 (open a warehouse)
	{
		vector<unsigned> candidates;
		for (unsigned w = 0; w < in.Warehouses(); w++)
			if (!sol->Load(w) && in.FixedCost(w))
				candidates.push_back(w);

		if (candidates.empty())
			return 0;

		unsigned w = candidates[rand() % candidates.size()];

		closing_forbidden->insert(w);
		invalid_warehouses->insert(w);

		break;
	}
	case 3: // Perturbation 3 (close one warehouse and open one warehouse)
	{
		vector<unsigned> candidates;
		for (unsigned w = 0; w < in.Warehouses(); w++)
			if (sol->Load(w) && in.FixedCost(w))
				candidates.push_back(w);

		if (candidates.empty())
			return 0;

		unsigned w1 = candidates[rand() % candidates.size()];

		candidates.clear();
		for (unsigned w = 0; w < in.Warehouses(); w++)
			if (!sol->Load(w) && in.FixedCost(w) && sol->ResidualCapacity(w) >= sol->Load(w1))
				candidates.push_back(w);

		if (candidates.empty())
			return 0;

		unsigned w2 = candidates[rand() % candidates.size()];

		while (!sol->supplied_stores[w1].empty())
		{
			unsigned s = *(sol->supplied_stores[w1].begin());
			unsigned q = sol->Supply(s, w1);
			sol->RevokeAssignment(s, w1, q);
			sol->Assign(s, w2, q);
		}

		opening_forbidden->insert(w1);
		closing_forbidden->insert(w2);
		invalid_warehouses->insert(w2);

		break;
	}
	case 4: // Perturbation 4 (close one warehouse and open two warehouses)
	{
		unsigned best_fc_improvement = 0;
		unsigned best_w1, best_w2, best_w3;
		for (unsigned w1 = 0; w1 < in.Warehouses(); w1++)
			if (sol->Load(w1) && in.FixedCost(w1))
				for (unsigned w2 = 0; w2 < in.Warehouses(); w2++)
					if (!sol->Load(w2) && in.FixedCost(w2) && in.FixedCost(w2) < in.FixedCost(w1))
						for (unsigned w3 = w2 + 1; w3 < in.Warehouses(); w3++)
						{
							unsigned fc_improvement = in.FixedCost(w1) - (in.FixedCost(w2) + in.FixedCost(w3));
							if (!sol->Load(w3) && in.FixedCost(w3) && in.Capacity(w2) + in.Capacity(w3) >= sol->Load(w1) && fc_improvement > best_fc_improvement)
							{
								best_fc_improvement = fc_improvement;
								best_w1 = w1;
								best_w2 = w2;
								best_w3 = w3;
							}
						}

		if (!best_fc_improvement)
			return 0;

		while (!sol->supplied_stores[best_w1].empty())
		{
			unsigned s = *(sol->supplied_stores[best_w1].begin());

			sol->RevokeAssignment(s, best_w1, sol->Supply(s, best_w1));

			if (sol->ResidualCapacity(best_w2))
				if (sol->ResidualCapacity(best_w3))
					if (in.SupplyCost(s, best_w2) < in.SupplyCost(s, best_w3))
					{
						sol->Assign(s, best_w2, min(sol->ResidualAmount(s), sol->ResidualCapacity(best_w2)));
						if (sol->ResidualAmount(s))
							sol->Assign(s, best_w3, sol->ResidualAmount(s));
					}
					else
					{
						sol->Assign(s, best_w3, min(sol->ResidualAmount(s), sol->ResidualCapacity(best_w3)));
						if (sol->ResidualAmount(s))
							sol->Assign(s, best_w2, sol->ResidualAmount(s));
					}
				else
					sol->Assign(s, best_w2, sol->ResidualAmount(s));
			else
				sol->Assign(s, best_w3, sol->ResidualAmount(s));
		}

		opening_forbidden->insert(best_w1);
		closing_forbidden->insert(best_w2);
		closing_forbidden->insert(best_w3);
		invalid_warehouses->insert(best_w2);
		invalid_warehouses->insert(best_w3);

		break;
	}
	default: // Perturbation 5 (open one warehouse and close two warehouses)
	{
		unsigned best_fc_improvement = 0;
		unsigned best_w1, best_w2, best_w3;
		for (unsigned w1 = 0; w1 < in.Warehouses(); w1++)
			if (!sol->Load(w1) && in.FixedCost(w1))
				for (unsigned w2 = 0; w2 < in.Warehouses(); w2++)
					if (sol->Load(w2) && in.Capacity(w1) > sol->Load(w2) && in.FixedCost(w2) && in.FixedCost(w1) < in.FixedCost(w2))
						for (unsigned w3 = w2 + 1; w3 < in.Warehouses(); w3++)
						{
							unsigned fc_improvement = in.FixedCost(w2) + in.FixedCost(w3) - in.FixedCost(w1);
							if (sol->Load(w3) && in.FixedCost(w3) && in.Capacity(w1) >= sol->Load(w2) + sol->Load(w3) && fc_improvement > best_fc_improvement)
							{
								bool compatible = true;
								for (auto it = sol->supplied_stores[w2].begin(); compatible && it != sol->supplied_stores[w2].end(); ++it)
								{
									unsigned s1 = *it;
									for (auto it2 = sol->supplied_stores[w3].begin(); compatible && it2 != sol->supplied_stores[w3].end(); ++it2)
									{
										unsigned s2 = *it2;
										if (in.Incompatible(s1, s2))
											compatible = false;
									}
								}
								if (compatible)
								{
									best_fc_improvement = fc_improvement;
									best_w1 = w1;
									best_w2 = w2;
									best_w3 = w3;
								}
							}
						}

		if (!best_fc_improvement)
			return 0;

		while (!sol->supplied_stores[best_w2].empty())
		{
			unsigned s = *(sol->supplied_stores[best_w2].begin());

			sol->RevokeAssignment(s, best_w2, sol->Supply(s, best_w2));
			sol->Assign(s, best_w1, sol->ResidualAmount(s));
		}
		while (!sol->supplied_stores[best_w3].empty())
		{
			unsigned s = *(sol->supplied_stores[best_w3].begin());

			sol->RevokeAssignment(s, best_w3, sol->Supply(s, best_w3));
			sol->Assign(s, best_w1, sol->ResidualAmount(s));
		}

		closing_forbidden->insert(best_w1);
		opening_forbidden->insert(best_w2);
		opening_forbidden->insert(best_w3);
		invalid_warehouses->insert(best_w1);
	}
	}

	return perturbation;
}

void WL_MRILS::MineElite()
{
	if (elite.size() > 1)
	{
		unsigned m_sup = max(2, (int)(min_sup * elite.size()));

		vector<vector<unsigned>> min_supply(in.Stores(), vector<unsigned>(in.Warehouses(), INT_MAX));

		Dataset *dataset = new Dataset;
		for (auto it = elite.begin(); it != elite.end(); ++it)
		{
			WL_Solution sol = *it;
			set<int> transaction;
			for (unsigned w = 0; w < in.Warehouses(); w++)
				for (auto it2 = sol.supplied_stores[w].begin(); it2 != sol.supplied_stores[w].end(); ++it2)
				{
					unsigned s = *it2;
					unsigned index = w * in.Stores() + s; // maps 2D matrix cell indices to a vector index
					transaction.insert(index);
					min_supply[s][w] = min(min_supply[s][w], sol.Supply(s, w));
				}
			dataset->push_back(transaction);
		}

		FISet *frequentItemsets = fpmax(dataset, m_sup, n_patterns);

		patterns.clear();
		for (FISet::iterator it = frequentItemsets->begin(); it != frequentItemsets->end(); ++it)
		{
			vector<Supply> pattern;

			for (std::set<int>::iterator it2 = it->begin(); it2 != it->end(); ++it2)
			{
				unsigned index = *it2;

				unsigned w = index / in.Stores();
				unsigned s = index % in.Stores();
				unsigned q = min_supply[s][w];

				pattern.push_back({w, s, q});
			}

			patterns.push_back(pattern);
		}
	}
}

// Returns a reduced version of instance `in` based on the pattern indexed by `p`
WL_Instance WL_MRILS::ReducedInstance(unsigned p)
{
	if (p == reduced_instances.size())
	{
		WL_Instance reduced_instance(in, patterns[p]);
		reduced_instances.push_back(reduced_instance);
	}

	return reduced_instances[p];
}