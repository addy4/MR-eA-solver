// Copyright (C) 2022  Marcelo R. H. Maia <mmaia@ic.uff.br, marcelo.h.maia@ibge.gov.br>


#include <fstream>
#include <iostream>

#include "WL_Instance.h"

// Reads instance from file
WL_Instance::WL_Instance(string file_name)
		: reduction_opening_cost(0), reduction_supply_cost(0)
{	
	const unsigned MAX_DIM = 100;
	unsigned w, s, s2;
	char ch, buffer[MAX_DIM];

	ifstream is(file_name);
	if(!is)
	{
		cerr << "Cannot open input file " <<	file_name << endl;
		exit(1);
	}
	
	is >> buffer >> ch >> warehouses >> ch;
	is >> buffer >> ch >> stores >> ch;
	
	capacity.resize(warehouses);
	fixed_cost.resize(warehouses);
	amount_of_goods.resize(stores);
	supply_cost.resize(stores,vector<double>(warehouses));
	incompatible.resize(stores,vector<bool>(stores, false));
	w_incompatible.resize(warehouses,vector<bool>(stores, false));
	
	// read capacity
	is.ignore(MAX_DIM,'['); // read "... Capacity = ["
	for (w = 0; w < warehouses; w++)
		is >> capacity[w] >> ch;
	
	// read fixed costs	
	is.ignore(MAX_DIM,'['); // read "... FixedCosts = ["
	for (w = 0; w < warehouses; w++)
		is >> fixed_cost[w] >> ch;

	// read goods
	is.ignore(MAX_DIM,'['); // read "... Goods = ["
	for (s = 0; s < stores; s++)
		is >> amount_of_goods[s] >> ch;

	// read supply costs
	is.ignore(MAX_DIM,'['); // read "... SupplyCost = ["
	is >> ch; // read first '|'
	for (s = 0; s < stores; s++)
	{	 
		for (w = 0; w < warehouses; w++)
			is >> supply_cost[s][w] >> ch;
	}
	is >> ch >> ch;

	// read store incompatibilities
	unsigned incompatibilities;
	is >> buffer >> ch >> incompatibilities >> ch;	
	store_incompatibilities.resize(incompatibilities);
	is.ignore(MAX_DIM,'['); // read "... IncompatiblePairs = ["
	for (unsigned i = 0; i < incompatibilities; i++)
	{
		is >> ch >> s >> ch >> s2; 
		store_incompatibilities[i].first = s - 1;
		store_incompatibilities[i].second = s2 - 1;
		incompatible[s - 1][s2 - 1] = true;
		incompatible[s2 - 1][s - 1] = true;
	}
	is >> ch >> ch;
	
	is.close();
}

// Creates a reduced version of instance `in` based on the provided pattern
WL_Instance::WL_Instance(const WL_Instance& in, vector<Supply> pattern)
		: stores(in.stores), warehouses(in.warehouses), reduction_opening_cost(0), reduction_supply_cost(0), capacity(in.capacity), fixed_cost(in.fixed_cost), amount_of_goods(in.amount_of_goods), 
		supply_cost(in.supply_cost), store_incompatibilities(in.store_incompatibilities), incompatible(in.incompatible), w_incompatible(in.w_incompatible)
{
	for (unsigned i = 0; i < pattern.size(); i++)
	{
		reduction_opening_cost += fixed_cost[pattern[i].w];
		reduction_supply_cost += supply_cost[pattern[i].s][pattern[i].w] * pattern[i].q;
		fixed_cost[pattern[i].w] = 0;
		capacity[pattern[i].w] -= pattern[i].q;
		amount_of_goods[pattern[i].s] -= pattern[i].q;
		for (unsigned s = 0; s < stores; s++)
			if (incompatible[pattern[i].s][s])
				w_incompatible[pattern[i].w][s] = true;
	}
}