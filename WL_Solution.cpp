// Copyright (C) 2022  Marcelo R. H. Maia <mmaia@ic.uff.br, marcelo.h.maia@ibge.gov.br>


#include <iostream>

#include "WL_Solution.h"

// Creates an empty solution
WL_Solution::WL_Solution(WL_Instance& my_in)
	: in(my_in), supply_cost(0), opening_cost(0), supply(in.Stores(),vector<unsigned>(in.Warehouses(),0)),
		assigned_goods(in.Stores(),0), load(in.Warehouses(),0), 
		incompatibilities(in.Warehouses(),vector<unsigned>(in.Stores(),0))
{
	supplied_stores.resize(in.Warehouses());
}

// Creates a solution based on data from another solution `sol`
WL_Solution::WL_Solution(WL_Solution* sol)
	: supplied_stores(sol->supplied_stores), in(sol->in), 
		supply_cost(sol->supply_cost), opening_cost(sol->opening_cost), supply(sol->supply),
		assigned_goods(sol->assigned_goods), load(sol->load), 
		incompatibilities(sol->incompatibilities)
{
}

 // Assigns `q` goods of store `s` to warehouse `w`
void WL_Solution::Assign(unsigned s, unsigned w, unsigned q)
{
	if (!supply[s][w])
	{
		for (unsigned s2 = 0; s2 < in.Stores(); s2++)
			if (in.Incompatible(s, s2))
				incompatibilities[w][s2]++;
		
		supplied_stores[w].insert(s);
	}

	supply[s][w] += q;
	assigned_goods[s] += q;
	
	supply_cost += in.SupplyCost(s, w) * q;
	
	if (!load[w])
		opening_cost += in.FixedCost(w);
	
	load[w] += q;
}

// Revokes assignment `q` goods of store `s` to warehouse `w`
void WL_Solution::RevokeAssignment(unsigned s, unsigned w, unsigned q)
{	
	supply[s][w] -= q;
	assigned_goods[s] -= q;
	load[w] -= q;
	
	supply_cost -= in.SupplyCost(s, w) * q;
	if (!load[w])
		opening_cost -= in.FixedCost(w);

	if (!supply[s][w])
	{
		for (unsigned s2 = 0; s2 < in.Stores(); s2++)
			if (in.Incompatible(s, s2))
				incompatibilities[w][s2]--;
		
		supplied_stores[w].erase(s);
	}
}

// Total cost
double WL_Solution::Cost() const
{
	return SupplyCost() + OpeningCost();
}

double WL_Solution::SupplyCost() const
{
	return supply_cost + in.ReductionSupplyCost();
}

unsigned WL_Solution::OpeningCost() const
{
	return opening_cost + in.ReductionOpeningCost();
}

unsigned WL_Solution::ComputeViolations() const
{
	unsigned s, w, i, violations = 0;
	for (s = 0; s < in.Stores(); s++)
		if (assigned_goods[s] < in.AmountOfGoods(s))
			violations++;
	for (w = 0; w < in.Warehouses(); w++)
		if (load[w] > in.Capacity(w))
			violations++;
	for (i = 0; i < in.StoreIncompatibilities(); i++)
		for (w = 0; w < in.Warehouses(); w++)
			if (supply[in.StoreIncompatibility(i).first][w] > 0 
					&& supply[in.StoreIncompatibility(i).second][w] > 0)
				violations++; 
	return violations;
}

void WL_Solution::PrintCosts(ostream& os) const
{
	unsigned s, w;
	double cost = 0;
	for (s = 0; s < in.Stores(); s++)
		for (w = 0; w < in.Warehouses(); w++)
			if (supply[s][w] > 0)
			{
				cost += in.SupplyCost(s,w) * supply[s][w];
				os << "Moving " << supply[s][w] << " goods from warehourse " << w+1 
					<< " to store " << s+1 << ", cost " << supply[s][w] << "x" 
					<< in.SupplyCost(s,w) << " = " << supply[s][w] * in.SupplyCost(s,w)
					<< " (" << cost << ")" << endl;
			}
	for (w = 0; w < in.Warehouses(); w++)
		if (load[w] > 0)
		{
			cost += in.FixedCost(w);
			os << "Opening warehouse " << w+1 << ", cost " << in.FixedCost(w) 
				<< " (" << cost << ")" << endl;
		}
}	
	
void WL_Solution::PrintViolations(ostream& os) const
{
	unsigned s, w, i;
	for (s = 0; s < in.Stores(); s++)
		if (assigned_goods[s] < in.AmountOfGoods(s))
			os << "Goods of store " << s+1 << " are not moved completely (ammount = "
				<< in.AmountOfGoods(s) << ", moved = " << assigned_goods[s] << ")" << endl;
	for (w = 0; w < in.Warehouses(); w++)
		if (load[w] > in.Capacity(w))
			os << "Goods of warehouses " << w+1 << " exceed its capacity (capacity = " 
				<< in.Capacity(w) << ", moved = " << load[w] << ")" << endl;
	for (i = 0; i < in.StoreIncompatibilities(); i++)
		for (w = 0; w < in.Warehouses(); w++)
			if (supply[in.StoreIncompatibility(i).first][w] > 0 
					&& supply[in.StoreIncompatibility(i).second][w] > 0)
				os << "Warehouses " << w+1 << " supplies incompatible stores " 
					<< in.StoreIncompatibility(i).first+1 << " and "
					<< in.StoreIncompatibility(i).second+1 << endl;
}	

void WL_Solution::Print(ostream& os) const
{
	unsigned s, w;
	bool first = true;
	
	os << "{";
	
	for (s = 0; s < in.Stores(); s++)
		for (w = 0; w < in.Warehouses(); w++)
			if (supply[s][w] > 0)
			{
				if (!first)
					os << ", ";
				os << "(" << s+1 << "," << w+1 << "," << supply[s][w] << ")";
				first = false;
			}
	
	os << "}" << endl;
}

// Returns a copy of this solution
WL_Solution*  WL_Solution::Copy()
{
	return new WL_Solution(this);
}