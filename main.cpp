// Copyright (C) 2022  Marcelo R. H. Maia <mmaia@ic.uff.br, marcelo.h.maia@ibge.gov.br>


#include <fstream>
#include <iomanip>
#include <iostream>

#include "WL_MRILS.h"

using namespace std;

int main(int argc, char* argv[])
{
	string instance;
	if (argc != 5)
	{
		cerr << "Usage: " << argv[0] << " <input_file> <solution_file> <timeout_seconds> <random_seed>" << endl
			<< "Input file in .dzn format." << endl;
		exit(1);
	}

	WL_Instance in(argv[1]);
	unsigned timeout = stoul(argv[3]);
	unsigned seed = stoul(argv[4]);
	
	// Solver params
	unsigned elite_size, max_patterns, ils_maxiter;
	double min_sup, stabi_param, ils_accept;
	bool random_opening;
	
	if (in.Warehouses() <= 150)
	{
		random_opening = true;
		ils_maxiter = 100;
		ils_accept = 1.01;
		
		elite_size = 5;
		max_patterns = 10;
		min_sup = 0.4;
		stabi_param = 0.07;
	}
	else if (in.Warehouses() <= 600)
	{
		random_opening = false;
		ils_maxiter = 200;
		ils_accept = 1.01;
		
		elite_size = 10;
		max_patterns = 6;
		min_sup = 0.9;
		stabi_param = 0.03;
	}
	else if (in.Warehouses() <= 1400)
	{
		random_opening = false;
		ils_maxiter = 100;
		ils_accept = 1.05;
		
		elite_size = 5;
		max_patterns = 6;
		min_sup = 0.8;
		stabi_param = 0.04;
	}
	else if (in.Warehouses() <= 2000)
	{
		random_opening = false;
		ils_maxiter = 100;
		ils_accept = 1.05;
		
		elite_size = 5;
		max_patterns = 6;
		min_sup = 0.8;
		stabi_param = 0.03;
	}
	else
	{
		random_opening = false;
		ils_maxiter = 200;
		ils_accept = 1.02;
		
		elite_size = 5;
		max_patterns = 1;
		min_sup = 1.0;
		stabi_param = 0.04;
	}
	
	srand(seed);
	
	WL_MRILS solver(in, timeout, seed, elite_size, stabi_param, min_sup, max_patterns, random_opening, ils_maxiter, ils_accept);
	solver.Run();
	
	WL_Solution* sol = solver.Best();
	
	ofstream out(argv[2]);
	sol->Print(out);
	out << "TimeToBest: " << setprecision(1) << fixed << solver.TimeBest() << endl;
	out.close();
	cout << "\nNumber of violations: " << sol->ComputeViolations() << endl;
	cout << "Cost: " << setprecision(2) << fixed << sol->Cost() << " = " << sol->SupplyCost() << " (supply cost) + " 
			 << sol->OpeningCost() << " (opening cost)" << endl;
	cout << "Time to reach best solution: " << setprecision(1) << solver.TimeBest() << " s" << endl;
	
	delete sol;
			 
	return 0;
}

