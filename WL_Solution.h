// Copyright (C) 2022  Marcelo R. H. Maia <mmaia@ic.uff.br, marcelo.h.maia@ibge.gov.br>


#ifndef _WL_SOLUTION
#define _WL_SOLUTION

#include <unordered_set>
#include <vector>

#include "WL_Instance.h"

// Problem solution
class WL_Solution 
{
public:
	WL_Solution(WL_Instance& i);
	WL_Solution(WL_Solution* sol);
	unsigned Supply(unsigned s, unsigned w) const { return supply[s][w]; }
	unsigned Load(unsigned w) const { return load[w]; }
	unsigned ResidualCapacity(unsigned w) const { return in.Capacity(w) - load[w]; }
	unsigned AssignedGoods(unsigned s) const { return assigned_goods[s]; }
	unsigned ResidualAmount(unsigned s) const { return in.AmountOfGoods(s) - assigned_goods[s]; }
	unsigned Incompatibilities(unsigned w, unsigned s) const { return in.WarehouseIncompatible(w, s) ? incompatibilities[w][s] + 2 : incompatibilities[w][s]; }
	void Assign(unsigned s, unsigned w, unsigned q); // assign q goods of s to w
	void RevokeAssignment(unsigned s, unsigned w, unsigned q); // revoke assignment of q goods of s to w
	double Cost() const;
	double SupplyCost() const;
	unsigned OpeningCost() const;
	unsigned ComputeViolations() const;
	void PrintCosts(ostream& os) const;
	void PrintViolations(ostream& os) const;
	void Print(ostream& os) const;
	WL_Solution* Copy();	// returns a copy of this solution
	vector<unordered_set<unsigned>> supplied_stores; //	set of supplied stores for each warehouse (for faster access)
private:
	WL_Instance& in;
	double supply_cost;
	unsigned opening_cost;
	vector<vector<unsigned>> supply;	 // main data
	vector<unsigned> assigned_goods;	 // quantity of goods of each store already assigned to warehouses
	vector<unsigned> load;	 // quantity of goods of each warehouse assigned to stores
	vector<vector<unsigned>> incompatibilities; //	warehouse/store incompatibility count matrix based on current assignment
	// NOTE: opening is implicit, based on load > 0
};

#endif