// Copyright (C) 2022  Marcelo R. H. Maia <mmaia@ic.uff.br, marcelo.h.maia@ibge.gov.br>


#ifndef _WL_INSTANCE
#define _WL_INSTANCE

#include <string>
#include <vector>

using namespace std;

// Supply structure (`q` goods supplied to store `s` by warehouse `w`)
struct Supply
{
    unsigned w, s, q;
};

// Problem input instance
class WL_Instance 
{
public:
	WL_Instance(string file_name);
	WL_Instance(const WL_Instance& in, vector<Supply> pattern);
	unsigned Stores() const { return stores; }
	unsigned Warehouses() const { return warehouses; }
	unsigned ReductionOpeningCost() const { return reduction_opening_cost; }
	double ReductionSupplyCost() const { return reduction_supply_cost; }
	unsigned Capacity(unsigned w) const { return capacity[w]; }
	unsigned FixedCost(unsigned w) const { return fixed_cost[w]; }
	unsigned AmountOfGoods(unsigned s) const { return amount_of_goods[s]; }
	double SupplyCost(unsigned s, unsigned w) const { return supply_cost[s][w]; }
	unsigned StoreIncompatibilities() const { return store_incompatibilities.size(); }
	pair<unsigned, unsigned> StoreIncompatibility(unsigned i) const { return store_incompatibilities[i]; }
	bool Incompatible(unsigned s1, unsigned s2) const { return incompatible[s1][s2]; }
	bool WarehouseIncompatible(unsigned w, unsigned s) const { return w_incompatible[w][s]; }
 private:
	unsigned stores, warehouses, reduction_opening_cost;
	double reduction_supply_cost;
	vector<unsigned> capacity;
	vector<unsigned> fixed_cost;
	vector<unsigned> amount_of_goods;
	vector<vector<double>> supply_cost;
	vector<pair<unsigned, unsigned>> store_incompatibilities;
	vector<vector<bool>> incompatible; //	store/store incompatibility matrix
	vector<vector<bool>> w_incompatible; //	warehouse/store incompatibility matrix
};

#endif