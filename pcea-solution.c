#include "pcea-solution.h"

void test()
{
	printf("C code\n");
}

void readCapacity(int *capacities_mr, int ware_houses)
{
	capacities = capacities_mr;
	for (int j = 0; j < ware_houses; j++)
	{
		printf("%d\n", capacities[j]);
	}
}

void readFixedcosts(int *fixedcosts_mr, int ware_houses)
{
	fixedcosts = fixedcosts_mr;
	for (int j = 0; j < ware_houses; j++)
	{
		printf("%d\n", fixedcosts[j]);
	}
}

void readDemands(int *storedemands_mr, int store_units)
{
	goods = storedemands_mr;
	for (int j = 0; j < store_units; j++)
	{
		printf("%d\n", goods[j]);
	}
}

void readSupplycosts(int *supply_costs, int store_units, int ware_houses)
{
	supplycosts = supply_costs;
	for (int i = 0; i < store_units; i++)
	{
		for (int j = 0; j < ware_houses; j++)
		{
			printf("%d\n", supplycosts[ware_houses * i + j]);
		}
	}
	return;
}

void readData(char *inputfilename)
{
	char temp[20];

	fp = fopen(inputfilename, "r");
	if (fp == NULL)
	{
		printf("FILE OPEN FAILED - EXITING");
		exit(1);
	}

	// Read nwarehouses
	fscanf(fp, "%s", temp);
	fscanf(fp, "%3c", temp);
	fscanf(fp, "%d", &warehouses);
	fscanf(fp, "%c", &ch);

	// Read nstores
	fscanf(fp, "%s", temp);
	fscanf(fp, "%3c", temp);
	fscanf(fp, "%d", &stores);
	fscanf(fp, "%c", &ch);

	// Read blank line
	fgets(temp, sizeof(temp), fp);

	// Read capacities (array of size=warehouses)

	// first allocate space for capacities array
	capacities = (int *)malloc(warehouses * sizeof(int));
	fscanf(fp, "%s", temp);
	fscanf(fp, "%3c", temp);
	ch = fgetc(fp); // read '['
	for (int i = 0; i < warehouses; i++)
	{
		fscanf(fp, "%d", &capacities[i]);
		ch = fgetc(fp);
	}
	fscanf(fp, "%2c", temp); // read '];'

	// Read fixedcosts (array of size=warehouses)

	//     first allocate space for fixedcosts array
	fixedcosts = (int *)malloc(warehouses * sizeof(int));

	//	   then read from file
	fscanf(fp, "%s", temp);
	fscanf(fp, "%3c", temp);
	ch = fgetc(fp); // read '['
	for (int i = 0; i < warehouses; i++)
	{
		fscanf(fp, "%d", &fixedcosts[i]);
		ch = fgetc(fp);
	}
	fscanf(fp, "%2c", temp); // read '];'

	// Read Goods (array of size=stores)

	//     first allocate space for goods array
	goods = (int *)malloc(stores * sizeof(int));

	//	   then read from file
	fscanf(fp, "%s", temp);
	fscanf(fp, "%3c", temp);
	ch = fgetc(fp); // read '['
	for (int i = 0; i < stores; i++)
	{
		fscanf(fp, "%d", &goods[i]);
		ch = fgetc(fp);
	}
	fscanf(fp, "%2c", temp); // read '];'
	int i = 0, j;

	// Read SupplyCosts (array of size=stores x warehouses)

	//     first allocate space for supplycosts array
	supplycosts = (int *)malloc(stores * warehouses * sizeof(int));

	//	   then read from file
	fscanf(fp, "%s", temp);
	fscanf(fp, "%4c", temp);

	ch = fgetc(fp); // first '|'

	i = 0;
	for (j = 0; j < warehouses; j++)
	{
		fscanf(fp, "%d", supplycosts + warehouses * i + j);
		ch = fgetc(fp);
	}

	for (i = 1; i < stores; i++)
	{
		j = 0;
		fscanf(fp, "%s", temp);
		char *ptr = strrchr(temp, '|');
		++ptr;
		*(supplycosts + warehouses * i + j) = atoi(ptr);

		for (j = 1; j < warehouses; j++)
		{
			fscanf(fp, "%d", supplycosts + warehouses * i + j);
			ch = fgetc(fp);
		}
	}
	fscanf(fp, "%3c", temp); // read '];'

	// Read blank line
	fgets(temp, sizeof(temp), fp);

	// Read incompatibilities
	fscanf(fp, "%s", temp);
	fscanf(fp, "%s", temp);
	fscanf(fp, "%d", &incompatibilities);
	fscanf(fp, "%c", &ch);

	// Read Incompatible Pairs

	// First allocate space for these pairs
	incompatiblepairs = (pairs *)malloc(2 * incompatibilities * sizeof(pairs));

	// Then read the file...
	fscanf(fp, "%s", temp);
	fscanf(fp, "%s", temp);
	fscanf(fp, "%s", temp);

	int k = 0;
	for (int i = 0; i < incompatibilities; i++)
	{
		fscanf(fp, "%d", &incompatiblepairs[k].x);
		fscanf(fp, "%s", temp);
		fscanf(fp, "%d", &incompatiblepairs[k].y);
		fscanf(fp, "%s", temp);
		incompatiblepairs[k].x -= 1;
		incompatiblepairs[k].y -= 1;
		k++;
		incompatiblepairs[k].x = incompatiblepairs[k - 1].y;
		incompatiblepairs[k].y = incompatiblepairs[k - 1].x;
		k++;
	}
	qsort(incompatiblepairs, k, sizeof(pairs), compare1);

	/* Other allocations */

	// Allocate space for the solution, for use elsewhere
	sol = (int *)malloc(warehouses * stores * sizeof(int));

	// sol1: initially assigned a size of (stores+warehouses), can be extended if need be
	sol1 = (pairs *)malloc((stores + warehouses) * sizeof(pairs));

	// Copy of capacities and goods for use inside fitness
	fcapacities = (int *)malloc(warehouses * sizeof(int));
	demand = (int *)malloc(stores * sizeof(int));

	// Allocate space for fs in	the fitness() function
	openwarehouses = (int *)malloc(warehouses * sizeof(int));

	// Allocate space for gBestSolution, pop, o1, p1, p2, remainingconflicts
	gBestSolution = (int *)malloc((stores + warehouses) * sizeof(int));
	pop = (int *)malloc(POPSIZE * (stores + warehouses) * sizeof(int));
	o1 = (int *)malloc((stores > warehouses ? stores : warehouses) * sizeof(int));
	o2 = (int *)malloc((stores > warehouses ? stores : warehouses) * sizeof(int));
	p1 = (int *)malloc((stores > warehouses ? stores : warehouses) * sizeof(int));
	p2 = (int *)malloc((stores > warehouses ? stores : warehouses) * sizeof(int));

	// Initialize totalDemand
	totalDemand = 0;
	for (int i = 0; i < stores; i++)
		totalDemand += goods[i];

	fclose(fp);
}

int notcorrectgbest()
{
	int *A, *B, i = 0, x = 0;
	A = (int *)malloc((stores) * sizeof(int));
	B = (int *)malloc((warehouses) * sizeof(int));
	for (i = 0; i < stores; i++)
		A[i] = 1;
	for (i = 0; i < warehouses; i++)
		B[i] = 1;
	for (i = 0; i < stores; i++)
		A[gBestSolution[i]]--;
	for (i = 0; i < warehouses; i++)
		B[gBestSolution[i + stores]]--;
	for (i = 0; i < stores; i++)
		x += A[i];
	for (i = 0; i < warehouses; i++)
		x += B[i];
	free(A);
	free(B);
	return x;
}

// Free POP memory
void freepop()
{
	free(pop);
}

/* f[] and v[] can be static arrays */
void displaypop()
{
	for (int i = 0; i < POPSIZE; i++)
	{
		for (int j = 0; j < (stores + warehouses); j++)
			printf("%d ", *(pop + i * (stores + warehouses) + j));
		printf(" f<%d> v<%d>\n", popFitness[i], violations[i]);
	}
}

void initpop()
{

	int i = 0, j = 0, k = 0, t;
	int x, y;

	// initialize population to all zeros
	for (i = 0; i < POPSIZE; i++)
		for (j = 0; j < (stores + warehouses); j++)
			*(pop + i * (stores + warehouses) + j) = 0;

	// Build a random permutation of the first solution

	// Initialize the first row of the population array to stores, then warehouses
	for (j = 0; j < stores; j++)
		*(pop + j) = j;
	k = 0;
	for (j = stores; j < (stores + warehouses); j++)
	{
		*(pop + j) = k;
		k++;
	}

	// 	(a) random permutation of stores
	for (j = 0; j <= stores; j++)
	{
		x = rand() % (stores); // x and y are the indices of the stores to swap
		while ((y = rand() % stores) == x)
			;
		t = *(pop + x);
		*(pop + x) = *(pop + y);
		*(pop + y) = t;
	}

	// 	(b) random permutation of warehouses
	for (j = 0; j <= warehouses; j++)
	{
		x = rand() % warehouses + stores; // x and y are the indices of the warehouses to swap
		while ((y = rand() % warehouses + stores) == x)
			;
		t = *(pop + x);
		*(pop + x) = *(pop + y);
		*(pop + y) = t;
	}

	// Make random permutations of the remaining solutions in the population
	for (i = 1; i < POPSIZE; i++)
	{
		srand(i);

		// Copy previous row into this row, as is
		for (j = 0; j < (stores + warehouses); j++)
			*(pop + i * (stores + warehouses) + j) = *(pop + (i - 1) * (stores + warehouses) + j);

		// 	Then make a random permutation of this row
		// (a) random permutation of stores, as before
		for (j = 0; j < stores; j++)
		{
			x = rand() % stores;
			while ((y = rand() % stores) == x)
				;
			t = *(pop + i * (stores + warehouses) + x);
			*(pop + i * (stores + warehouses) + x) = *(pop + i * (stores + warehouses) + y);
			*(pop + i * (stores + warehouses) + y) = t;
		}

		// (b) random permutation of warehouses, as before
		for (j = 0; j < warehouses; j++)
		{
			x = rand() % warehouses + stores;
			while ((y = rand() % warehouses + stores) == x)
				;
			t = *(pop + i * (stores + warehouses) + x);
			*(pop + i * (stores + warehouses) + x) = *(pop + i * (stores + warehouses) + y);
			*(pop + i * (stores + warehouses) + y) = t;
		}
	}

	// Compute fitness and violations, update arrays
	int min_no_of_violations = INF;
	int min_index = -1;

	for (i = 0; i < POPSIZE; i++)
	{
		fitness(pop + i * (stores + warehouses), &popFitness[i], &violations[i]);
		if (violations[i] < min_no_of_violations)
		{
			min_no_of_violations = violations[i];
			min_index = i;
		}
		else if ((violations[i] == min_no_of_violations) && (popFitness[i] < popFitness[min_index]))
			min_index = i;
	}

	//	Update gbestFitness, gBestSolution
	int gBestIndex = -1;
	int bestFitness = INF;

	if (min_no_of_violations > 0)
	{
		gBestIndex = min_index;
		bestFitness = popFitness[gBestIndex];
	}
	else
	{ // min_no_of_violations == 0
		bestFitness = popFitness[0];
		gBestIndex = 0;
		for (i = 1; i < POPSIZE; i++)
			if (violations[i] == 0)
				if (popFitness[i] < bestFitness)
				{
					gBestIndex = i;
					bestFitness = popFitness[i];
				}
	}

	for (int x = 0; x < (stores + warehouses); x++)
		gBestSolution[x] = *(pop + gBestIndex * (stores + warehouses) + x);
	if (notcorrectgbest())
	{
		printf("\nFail inside initpop()\n");
		exit(1);
	}
	gBestFitness = bestFitness;
	gBestViolations = min_no_of_violations;

	return;
}

void reinit()
{

	int i = 0, j = 0, k = 0, t;
	int x, y;

	// initialize population to all zeros
	for (i = 0; i < POPSIZE; i++)
		for (j = 0; j < (stores + warehouses); j++)
			*(pop + i * (stores + warehouses) + j) = 0;

	// Initialize the first solution to the gBestSolution
	for (j = 0; j < (stores + warehouses); j++)
		*(pop + j) = gBestSolution[j];

	// Make random permutations of the remaining solutions in the population
	for (i = 1; i < POPSIZE; i++)
	{

		// Copy previous row into this row, as is
		for (j = 0; j < (stores + warehouses); j++)
			*(pop + i * (stores + warehouses) + j) = *(pop + (i - 1) * (stores + warehouses) + j);

		// 	Then make a random permutation of this row
		// (a) random permutation of stores, as before
		for (j = 0; j < stores; j++)
		{
			x = rand() % stores;
			while ((y = rand() % stores) == x)
				;
			t = *(pop + i * (stores + warehouses) + x);
			*(pop + i * (stores + warehouses) + x) = *(pop + i * (stores + warehouses) + y);
			*(pop + i * (stores + warehouses) + y) = t;
		}

		// (b) random permutation of warehouses, as before
		for (j = 0; j < warehouses; j++)
		{
			x = rand() % warehouses + stores;
			while ((y = (rand() % warehouses) + stores) == x)
				;
			t = *(pop + i * (stores + warehouses) + x);
			*(pop + i * (stores + warehouses) + x) = *(pop + i * (stores + warehouses) + y);
			*(pop + i * (stores + warehouses) + y) = t;
		}
	}

	// Compute fitness and violations, update arrays
	int min_no_of_violations = INF;
	int min_index = -1;

	for (i = 0; i < POPSIZE; i++)
	{
		fitness(pop + i * (stores + warehouses), &popFitness[i], &violations[i]);
		if (violations[i] < min_no_of_violations)
		{
			min_no_of_violations = violations[i];
			min_index = i;
		}
		else if ((violations[i] == min_no_of_violations) && (popFitness[i] < popFitness[min_index]))
			min_index = i;
	}

	//	Update gbestFitness, gBestSolution
	int gBestIndex = -1;
	int bestFitness = INF;

	if (min_no_of_violations > 0)
	{
		gBestIndex = min_index;
		bestFitness = popFitness[gBestIndex];
	}
	else
	{ // min_no_of_violations == 0
		bestFitness = popFitness[0];
		gBestIndex = 0;
		for (i = 1; i < POPSIZE; i++)
			if (violations[i] == 0)
				if (popFitness[i] < bestFitness)
				{
					gBestIndex = i;
					bestFitness = popFitness[i];
				}
	}

	for (int x = 0; x < (stores + warehouses); x++)
		gBestSolution[x] = *(pop + gBestIndex * (stores + warehouses) + x);
	if (notcorrectgbest())
	{
		printf("\nFail inside reinit()\n");
		exit(1);
	}
	gBestFitness = bestFitness;
	gBestViolations = min_no_of_violations;

	return;
}

int findWH(int s, int *whidx, int wh[])
{

	int anycost = INF, lowestcostinfeasibleidx = -1;
	int bestcost = INF, bestidx = -1;

	for (int j = 0; j <= (*whidx); j++)
		if (fcapacities[wh[j]] > 0)
		{
			if (*(supplycosts + s * warehouses + wh[j]) < anycost)
			{
				anycost = *(supplycosts + s * warehouses + wh[j]);
				lowestcostinfeasibleidx = wh[j];
			}
			// check feasibility of this wh
			if (*(sol + s * warehouses + wh[j]) != -1)
			{
				if (*(supplycosts + s * warehouses + wh[j]) < bestcost)
				{
					bestcost = *(supplycosts + s * warehouses + wh[j]);
					bestidx = wh[j];
				}
			}
		}

	if (bestidx == -1)
	{ // nothing feasible so far
		// if possible open the next warehouse, increment whidx, return its idx
		if ((*whidx) < warehouses)
		{
			(*whidx)++;						  // move to the next warehouse in the permutation
			openwarehouses[wh[(*whidx)]] = 1; // and open it
			bestidx = wh[(*whidx)];			  // this is the warehouse to allocate to
		}
		else
		{ // if whidx has reached the end (i.e., all warehouses are open / whidx==warehouses)
			//	then return lowestcostinfeasibleidx
			bestidx = lowestcostinfeasibleidx;
		}
	}
	return bestidx;
}

void fitness(int str[], int *solcost, int *nviolations)
{
	// str[] is the chromosome

	int i, j, violationscount = 0;
	int st[stores], wh[warehouses];

	// Initialize all open warehouses to 0 (closed)
	for (i = 0; i < warehouses; i++)
		openwarehouses[i] = 0;

	// Make a copy of the capacities array, called fcapacities[]
	for (i = 0; i < warehouses; i++)
		fcapacities[i] = capacities[i];

	// Make a copy of the goods array, called demand[]
	for (i = 0; i < stores; i++)
		demand[i] = goods[i];

	// Initialize all sol entries to 0;
	for (i = 0; i < stores; i++)
		for (j = 0; j < warehouses; j++)
			*(sol + i * warehouses + j) = 0;

	// Initialize all sol1 entries to 0;
	solcount = 0;
	for (i = 0; i < (stores + warehouses); i++)
	{ // put the store number in x, wh in y
		sol1[i].x = sol1[i].y = -1;
		sol1[i].v = 0;
	}
	// Allocate in permutation order
	// DO NOT use i,j as loop variables hereon,
	// as we're using them for marking our best store and warehouse

	// Split into two arrays: stores and warehouses
	for (i = 0; i < stores; i++)
		st[i] = str[i];
	j = 0;
	for (i = stores; i < (stores + warehouses); i++)
	{
		wh[j] = str[i];
		j++;
	}

	// Open warehouses in permutation order until total demand can be satisfied
	int tempDemand = 0;
	j = 0;

	while (tempDemand <= totalDemand)
	{
		openwarehouses[wh[j]] = 1;
		tempDemand += capacities[wh[j]];
		j++;
	}

	int idx = 0;   // use to iterate through the st[] permutation
	int whidx = j; // use to iterate through the wh[] permutation, initialized to j
	i = j = 0;

	// Allocate stores to warehouses in permutation order

	while (idx < stores)
	{

		i = st[idx]; // i is the actual store number (keep)
		while (demand[i] != 0)
		{

			j = findWH(i, &whidx, wh);

			// If closed then open the warehouse, update data structures
			if (openwarehouses[j] == 0)
				openwarehouses[j] = 1;

			// Allocate (update demand, wh, mark incompatibilities, storeidx[i]
			if (demand[i] <= fcapacities[j])
			{
				if ((*(sol + i * warehouses + j)) == -1)
				{
					*(sol + i * warehouses + j) = 0;
					violationscount++;
				} // worst case
				*(sol + i * warehouses + j) += demand[i];
				// mark all incompatible stores -1 in this column (this warehouse)
				// set conflicting column elements to -1, if currently zero; otherwise leave as is
				// binarysearch for incompatiblepairs[p].x == i, search either side from there, setting all incompatible y's to -1
				int z = binarySearch(incompatiblepairs, 0, 2 * incompatibilities, i);
				if (z != -1)
				{
					for (int p = z; (incompatiblepairs[p].x == i) && (p < 2 * incompatibilities); p++)
					{
						int q = incompatiblepairs[p].y;
						if (*(sol + q * warehouses + j) == 0)
							*(sol + q * warehouses + j) = -1;
					}

					for (int p = z - 1; (incompatiblepairs[p].x == i) && (p >= 0); p--)
					{
						int q = incompatiblepairs[p].y;
						if (*(sol + q * warehouses + j) == 0)
							*(sol + q * warehouses + j) = -1;
					}
				}

				{
					sol1[solcount].x = i;
					sol1[solcount].y = j;
					sol1[solcount].v += demand[i];
				}
				fcapacities[j] -= demand[i];
				demand[i] = 0;
				solcount++;
			}
			else
			{ // if (demand[i] > fcapacities[j])

				if ((*(sol + i * warehouses + j)) == -1)
				{
					*(sol + i * warehouses + j) = 0;
					violationscount++;
				} // worst case
				*(sol + i * warehouses + j) += fcapacities[j];
				// set conflicting column elements to -1, if currently zero; otherwise leave as is
				int z = binarySearch(incompatiblepairs, 0, 2 * incompatibilities, i);
				if (z != -1)
				{
					for (int p = z; (incompatiblepairs[p].x == i) && (p < 2 * incompatibilities); p++)
					{
						int q = incompatiblepairs[p].y;
						if (*(sol + q * warehouses + j) == 0)
							*(sol + q * warehouses + j) = -1;
					}
					for (int p = z - 1; (incompatiblepairs[p].x == i) && (p >= 0); p--)
					{
						int q = incompatiblepairs[p].y;
						if (*(sol + q * warehouses + j) == 0)
							*(sol + q * warehouses + j) = -1;
					}
				}
				{
					sol1[solcount].x = i;
					sol1[solcount].y = j;
					sol1[solcount].v += fcapacities[j];
				}
				demand[i] -= fcapacities[j];
				fcapacities[j] = 0;
				solcount++;
			} // end else
		}

		idx++;
	}

	// For each open warehouse in sol[][] do:
	// 	compute_cost_of_WH(opening cost + cost of all current allocations)
	// For each open warehouse in sol[][] do:
	// Find the cost of allocation in another warehouse.

	*nviolations = violationscount;
	*solcost = ComputeCost();

	return;
}

void finalfitness(int str[], int *solcost, int *nviolations)
{
	// str[] is the chromosome

	int i, j, violationscount = 0;
	int st[stores], wh[warehouses];

	// Initialize all open warehouses to 0 (closed)
	for (i = 0; i < warehouses; i++)
		openwarehouses[i] = 0;

	// Make a copy of the capacities array, called fcapacities[]
	for (i = 0; i < warehouses; i++)
		fcapacities[i] = capacities[i];

	// Make a copy of the goods array, called demand[]
	for (i = 0; i < stores; i++)
		demand[i] = goods[i];

	// Initialize all sol entries to 0;
	for (i = 0; i < stores; i++)
		for (j = 0; j < warehouses; j++)
			*(sol + i * warehouses + j) = 0;

	// Initialize all sol1 entries to 0;
	solcount = 0;
	for (i = 0; i < (stores + warehouses); i++)
	{ // put the store number in x, wh in y
		sol1[i].x = sol1[i].y = -1;
		sol1[i].v = 0;
	}
	// Allocate in permutation order
	// DO NOT use i,j as loop variables hereon,
	// as we're using them for marking our best store and warehouse

	// Split into two arrays: stores and warehouses
	for (i = 0; i < stores; i++)
		st[i] = str[i];
	j = 0;
	for (i = stores; i < (stores + warehouses); i++)
	{
		wh[j] = str[i];
		j++;
	}

	// Open warehouses in permutation order until total demand can be satisfied
	int tempDemand = 0;
	j = 0;

	while (tempDemand <= totalDemand)
	{
		openwarehouses[wh[j]] = 1;
		tempDemand += capacities[wh[j]];
		j++;
	}

	int idx = 0;   // use to iterate through the st[] permutation
	int whidx = j; // use to iterate through the wh[] permutation, initialized to j
	i = j = 0;

	// Allocate stores to warehouses in permutation order

	while (idx < stores)
	{

		i = st[idx]; // i is the actual store number (keep)
		while (demand[i] != 0)
		{

			j = findWH(i, &whidx, wh);

			// If closed then open the warehouse, update data structures
			if (openwarehouses[j] == 0)
				openwarehouses[j] = 1;

			// Allocate (update demand, wh, mark incompatibilities, storeidx[i]
			if (demand[i] <= fcapacities[j])
			{
				if ((*(sol + i * warehouses + j)) == -1)
				{
					*(sol + i * warehouses + j) = 0;
					violationscount++;
				} // worst case
				*(sol + i * warehouses + j) += demand[i];
				// mark all incompatible stores -1 in this column (this warehouse)
				// set conflicting column elements to -1, if currently zero; otherwise leave as is
				// binarysearch for incompatiblepairs[p].x == i, search either side from there, setting all incompatible y's to -1
				int z = binarySearch(incompatiblepairs, 0, 2 * incompatibilities, i);
				if (z != -1)
				{
					for (int p = z; (incompatiblepairs[p].x == i) && (p < 2 * incompatibilities); p++)
					{
						int q = incompatiblepairs[p].y;
						if (*(sol + q * warehouses + j) == 0)
							*(sol + q * warehouses + j) = -1;
					}

					for (int p = z - 1; (incompatiblepairs[p].x == i) && (p >= 0); p--)
					{
						int q = incompatiblepairs[p].y;
						if (*(sol + q * warehouses + j) == 0)
							*(sol + q * warehouses + j) = -1;
					}
				}

				{
					sol1[solcount].x = i;
					sol1[solcount].y = j;
					sol1[solcount].v += demand[i];
				}
				fcapacities[j] -= demand[i];
				demand[i] = 0;
				solcount++;
			}
			else
			{ // if (demand[i] > fcapacities[j])

				if ((*(sol + i * warehouses + j)) == -1)
				{
					*(sol + i * warehouses + j) = 0;
					violationscount++;
				} // worst case
				*(sol + i * warehouses + j) += fcapacities[j];
				// set conflicting column elements to -1, if currently zero; otherwise leave as is
				int z = binarySearch(incompatiblepairs, 0, 2 * incompatibilities, i);
				if (z != -1)
				{
					for (int p = z; (incompatiblepairs[p].x == i) && (p < 2 * incompatibilities); p++)
					{
						int q = incompatiblepairs[p].y;
						if (*(sol + q * warehouses + j) == 0)
							*(sol + q * warehouses + j) = -1;
					}
					for (int p = z - 1; (incompatiblepairs[p].x == i) && (p >= 0); p--)
					{
						int q = incompatiblepairs[p].y;
						if (*(sol + q * warehouses + j) == 0)
							*(sol + q * warehouses + j) = -1;
					}
				}
				{
					sol1[solcount].x = i;
					sol1[solcount].y = j;
					sol1[solcount].v += fcapacities[j];
				}
				demand[i] -= fcapacities[j];
				fcapacities[j] = 0;
				solcount++;
			} // end else
		}

		idx++;
	}

	// For each open warehouse in sol[][] do:
	// 	compute_cost_of_WH(opening cost + cost of all current allocations)
	int costWH[warehouses];
	for (i = 0; i < warehouses; i++)
		costWH[i] = 0; // initalize all costs to zero

	for (i = 0; i < warehouses; i++)
		if (openwarehouses[i])
		{
			for (j = 0; j < stores; j++)
				if ((*(sol + j * warehouses + i)) != -1)
					costWH[i] += (*(sol + j * warehouses + i)) * (*(supplycosts + j * warehouses + i));
			costWH[i] += fixedcosts[i];
		}

	// For each open warehouse in sol[][] do:
	// Find the cost of allocation in another warehouse.
	for (i = 0; i < warehouses; i++)
		if (openwarehouses[i])
		{
			int replace = 0;
			int replacewith = -1;
			int bestcost = costWH[i];

			for (j = 0; j < warehouses; j++)
				if ((!openwarehouses[j]) && (capacities[j] >= capacities[i]))
				{
					// compute cost of allocating WH j instead of WH i
					int thiscost = 0;
					for (int k = 0; k < stores; k++)
						if ((*(sol + k * warehouses + i)) != -1)
							thiscost += (*(sol + k * warehouses + i)) * (*(supplycosts + k * warehouses + j));
					thiscost += fixedcosts[j];
					// if this cost is lower then update variables: replace, replacewith and lowestcost
					if (thiscost < bestcost)
					{
						replace = 1;
						replacewith = j;
						bestcost = thiscost;
					}
				}

			if (replace == 1)
			{
				j = replacewith;
				// open warehouse j, close warehouse i: column
				for (int k = 0; k < stores; k++)
				{
					int temp = (*(sol + k * warehouses + j));
					(*(sol + k * warehouses + j)) = (*(sol + k * warehouses + i));
					(*(sol + k * warehouses + i)) = temp;
				}

				for (int k = 0; k < solcount; k++)
					if (sol1[k].y == i)
						sol1[k].y = j;

				openwarehouses[i] = 0;
				openwarehouses[j] = 1;
			}
		}

	*nviolations = violationscount;
	*solcost = ComputeCost();

	return;
}

// Order and Alternation Crossover operators
// 1. Davis' Order Xover (p1 and p2 are copies of parents, o1 and 02 are the offsprings, MAX is the chromosome length
void crossover_order(int *p1, int *p2, int MAX)
{

	int i, j, *a;
	int temp, match, marker;
	int mark_l, mark_r;

	a = (int *)malloc(MAX * sizeof(int));

	for (i = 0; i < MAX; i++)
	{
		a[i] = 0;
		o1[i] = -1;
		o2[i] = -1;
	}

	while (((mark_l = rand() % MAX) == 0) || (mark_l == (MAX - 1)))
		;
	while (((mark_r = rand() % MAX) == mark_l) || (mark_r == 0) || (mark_r == (MAX - 1)))
		;

	if (mark_l > mark_r)
	{ // ensure that mark_l < mark_r
		temp = mark_l;
		mark_l = mark_r;
		mark_r = temp;
	}

	// Code for o1

	for (i = mark_l; i < mark_r; i++)
	{
		o1[i] = p1[i];
		a[p1[i]] = 1;
	}

	marker = mark_r;
	for (i = mark_r; i < MAX; i++)
	{
		if (a[p2[i]])
			;
		else
		{
			o1[marker] = p2[i];
			marker = (marker + 1) % MAX;
		}
	}

	for (i = 0; i < mark_r; i++)
	{
		if (a[p2[i]])
			;
		else
		{
			o1[marker] = p2[i];
			marker = (marker + 1) % MAX;
		}
	}

	// Code for o2

	for (i = 0; i < MAX; i++)
		a[i] = 0;

	for (i = mark_l; i < mark_r; i++)
	{
		o2[i] = p2[i];
		a[p2[i]] = 1;
	}

	marker = mark_r;
	for (i = mark_r; i < MAX; i++)
	{
		if (a[p1[i]])
			;
		else
		{
			o2[marker] = p1[i];
			marker = (marker + 1) % MAX;
		}
	}

	for (i = 0; i < mark_r; i++)
	{
		if (a[p1[i]])
			;
		else
		{
			o2[marker] = p1[i];
			marker = (marker + 1) % MAX;
		}
	}

	free(a);
}

// 2. Alternation Xover (p1 and p2 are copies of parents, o1 is the offspring, MAX is the chromosome length
void crossover_alternation(int *p1, int *p2, int MAX)
{

	int i, j;
	int *a = (int *)malloc(MAX * sizeof(int));
	int *temp = (int *)malloc(2 * MAX * sizeof(int));
	//int * r = (int *) malloc(MAX*sizeof(int));

	// Initialize o1[], a[]
	for (i = 0; i < MAX; i++)
	{
		a[i] = -1;
		o1[i] = -1;
	}

	// Build temp[]
	j = 0;
	for (i = 0; i < MAX; i++)
	{
		temp[j++] = p1[i];
		temp[j++] = p2[i];
	}

	// Eliminate duplicates, update a[i]
	int count = 0;
	for (i = 0; i < 2 * MAX; i++)
	{
		if (a[temp[i]] == -1)
		{
			a[temp[i]] = i;
			o1[count++] = temp[i];
		}
		if (count == MAX)
			break;
	}

	free(a);
	free(temp);
}

// Simple Swap Mutation
void mutation_swap(int *p1, int MAX)
{
	int i = rand() % MAX;
	int j, t;
	for (j = 0; j < MAX; j++)
		o1[j] = p1[j];
	while ((j = rand() % MAX) == i)
		;
	t = o1[i];
	o1[i] = o1[j];
	o1[j] = t;
}

// Binary search
int binarySearch(pairs arr[], int l, int r, int x)
{
	while (l <= r)
	{
		int m = l + (r - l) / 2;
		if (arr[m].x == x)
			return m;
		if (arr[m].x < x)
			l = m + 1;
		else
			r = m - 1;
	}
	return -1;
}

// Cost function
unsigned ComputeCost() // converted
{
	unsigned s, w, cost = 0;
	int wh[warehouses];

	for (w = 0; w < warehouses; w++)
		wh[w] = 0;

	for (int i = 0; i < solcount; i++)
	{
		s = sol1[i].x;
		w = sol1[i].y;
		int amt = sol1[i].v;

		wh[w] = 1;
		cost += (*(supplycosts + warehouses * s + w)) * amt;
	}

	for (w = 0; w < warehouses; w++)
		if (wh[w])
			cost += fixedcosts[w];

	return cost;
}

int _2_T()
{
	int i = rand() % POPSIZE;
	int j;
	while ((j = rand() % POPSIZE) == i)
		;
	// if both have violations, return index with lesser violation
	if ((violations[i] > 0) && (violations[j] > 0))
		return ((violations[i] < violations[j]) ? i : j);
	// if either one has zero violations (but not both) , return that index
	if (((violations[i] > 0) && (violations[j] == 0)) || ((violations[j] > 0) && (violations[i] == 0)))
		return ((violations[i] < violations[j]) ? i : j);
	// if both have zero violations, return index with lower fitness score
	if ((violations[i] == 0) && (violations[j] == 0))
		return ((popFitness[i] < popFitness[j]) ? i : j);
}

int _3_T()
{
	int i = rand() % POPSIZE, j, k, t;
	int f;
	int best = i;
	f = popFitness[i];
	while ((j = rand() % POPSIZE) == i)
		;
	while ((k = rand() % POPSIZE) == i || k == j)
		;

	t = (violations[i] < violations[j]) ? i : j;
	best = (violations[t] < violations[k]) ? t : k;

	if ((violations[i] == 0) && (violations[j] == 0) && (violations[k] == 0))
		return (((popFitness[i] < popFitness[j] ? i : j) < popFitness[k]) ? (popFitness[i] < popFitness[j] ? i : j) : k);

	return best;
}

void display(int *p, int MAX)
{
	int i = 0;
	printf("\n");
	for (i = 0; i < MAX; i++)
		printf("%d ", *(p + i));
}

/* Steady-state EA Loop */
int ealoop()
{ // returns 1 if the best solution improved this generation, 0 otherwise

	int p1, p2;
	int improvement = 0;
	int temp[stores + warehouses], ofs[stores + warehouses];
	int popsel = rand() % 100;
	int operation;
	int f, v, worstv, worstf;
	int minv = INF, minf = INF;

	// Select the first parent
	//p1 = _2_T();
	p1 = _3_T();

	if (popsel < STOREBIAS)
	{ // stores

		if (rand() % 100 < XPROB)
			operation = XOVER;
		else
			operation = MUTATION;

		if (operation == XOVER)
		{ // perform crossover, select the better fit of the two individuals

			// Select the second parent (distinct from the first)
			while ((p2 = _3_T()) == p1)
				;

			// Order Xover
			crossover_order(pop + p1 * (stores + warehouses), pop + p2 * (stores + warehouses), stores);

			for (int x = 0; x < stores; x++)
				temp[x] = o1[x];
			for (int x = stores; x < stores + warehouses; x++)
				temp[x] = *(pop + p1 * (stores + warehouses) + x);
			fitness(temp, &f, &v);
			minv = v;
			minf = f;
			for (int x = 0; x < stores + warehouses; x++)
				ofs[x] = temp[x];

			for (int x = 0; x < stores; x++)
				temp[x] = o1[x];
			for (int x = stores; x < stores + warehouses; x++)
				temp[x] = *(pop + p2 * (stores + warehouses) + x);
			fitness(temp, &f, &v);
			if ((v == minv) && (f < minf))
			{
				minf = f;
				for (int x = 0; x < stores + warehouses; x++)
					ofs[x] = temp[x];
			}
			if (v < minv)
			{
				minv = v;
				minf = f;
				for (int x = 0; x < stores + warehouses; x++)
					ofs[x] = temp[x];
			}

			for (int x = 0; x < stores; x++)
				temp[x] = o2[x];
			for (int x = stores; x < stores + warehouses; x++)
				temp[x] = *(pop + p1 * (stores + warehouses) + x);
			fitness(temp, &f, &v);
			if ((v == minv) && (f < minf))
			{
				minf = f;
				for (int x = 0; x < stores + warehouses; x++)
					ofs[x] = temp[x];
			}
			if (v < minv)
			{
				minv = v;
				minf = f;
				for (int x = 0; x < stores + warehouses; x++)
					ofs[x] = temp[x];
			}

			for (int x = 0; x < stores; x++)
				temp[x] = o2[x];
			for (int x = stores; x < stores + warehouses; x++)
				temp[x] = *(pop + p2 * (stores + warehouses) + x);
			fitness(temp, &f, &v);
			if ((v == minv) && (f < minf))
			{
				minf = f;
				for (int x = 0; x < stores + warehouses; x++)
					ofs[x] = temp[x];
			}
			if (v < minv)
			{
				minv = v;
				minf = f;
				for (int x = 0; x < stores + warehouses; x++)
					ofs[x] = temp[x];
			}
		}
		else
		{ // perform mutation

			// Swap Mutation
			mutation_swap(pop + p1 * (stores + warehouses), stores);

			for (int x = 0; x < stores; x++)
				temp[x] = o1[x];
			for (int x = stores; x < stores + warehouses; x++)
				temp[x] = *(pop + p1 * (stores + warehouses) + x);
			fitness(temp, &minf, &minv);
			for (int x = 0; x < stores + warehouses; x++)
				ofs[x] = temp[x];
		}
	}
	else
	{ // warehouses

		if (rand() % 100 < XPROB)
			operation = XOVER;
		else
			operation = MUTATION;

		if (operation == XOVER)
		{ // perform crossover, select the better fit of the two individuals

			// Select the second parent (distinct from the first)
			while ((p2 = _3_T()) == p1)
				;

			// Alternation Xover
			crossover_alternation(pop + p1 * (stores + warehouses) + stores, pop + p2 * (stores + warehouses) + stores, warehouses);

			minv = INF, minf = INF;
			for (int x = 0; x < stores; x++)
				temp[x] = *(pop + p1 * (stores + warehouses) + x);
			for (int x = stores; x < stores + warehouses; x++)
				temp[x] = o1[x - stores];
			fitness(temp, &f, &v);
			minv = v;
			minf = f;
			for (int x = 0; x < stores + warehouses; x++)
				ofs[x] = temp[x];

			for (int x = 0; x < stores; x++)
				temp[x] = *(pop + p2 * (stores + warehouses) + x);
			for (int x = stores; x < stores + warehouses; x++)
				temp[x] = o1[x - stores];
			fitness(temp, &f, &v);
			if ((v == minv) && (f < minf))
			{
				minf = f;
				for (int x = 0; x < stores + warehouses; x++)
					ofs[x] = temp[x];
			}
			if (v < minv)
			{
				minv = v;
				minf = f;
				for (int x = 0; x < stores + warehouses; x++)
					ofs[x] = temp[x];
			}
		}
		else
		{ // perform mutation

			// Swap Mutation
			mutation_swap(pop + p1 * (stores + warehouses) + stores, warehouses);

			for (int x = 0; x < stores; x++)
				temp[x] = *(pop + p1 * (stores + warehouses) + x);
			for (int x = stores; x < stores + warehouses; x++)
				temp[x] = o1[x - stores];
			fitness(temp, &minf, &minv);

			for (int x = 0; x < stores + warehouses; x++)
				ofs[x] = temp[x];
		}
	}

	// minv, minf contain the corresponding violations and fitness for ofs[]

	// if ofs is better than gBest then update o1
	if ((minv == gBestViolations) && (minf < gBestFitness))
	{
		gBestFitness = minf;
		for (int x = 0; x < stores + warehouses; x++)
			gBestSolution[x] = ofs[x];
		if (notcorrectgbest())
		{
			printf("\nFail inside EAloop():ofs1\n");
			exit(1);
		}
		improvement = 1;
	}
	if (minv < gBestViolations)
	{
		gBestViolations = minv;
		gBestFitness = minf;
		for (int x = 0; x < stores + warehouses; x++)
			gBestSolution[x] = ofs[x];
		if (notcorrectgbest())
		{
			printf("\nFail inside EALoop():ofs2\n");
			exit(1);
		}
		improvement = 1;
	}

	// find worst
	int wf = -1, wv = -1;
	int worstidx;
	for (int x = 0; x < POPSIZE; x++)
	{
		// find the solution worstidx for which number of violations is max, and corresponding
		// fitness is also highest
		if ((violations[x] == wv) && (popFitness[x] > wf))
		{
			wf = popFitness[x];
			worstidx = x;
		}
		if (violations[x] > wv)
		{
			wv = violations[x];
			wf = popFitness[x];
			worstidx = x;
		}
	}

	// if ofs is better than worst then update worst
	if (((minv == worstv) && (minf < worstf)) || (minv < worstv))
	{
		//worstf = minf;
		for (int x = 0; x < stores + warehouses; x++)
			*(pop + worstidx * (stores + warehouses) + x) = ofs[x];
		violations[worstidx] = minv;
		popFitness[worstidx] = minf;
	}

	return improvement;
}

// qsort compare1() routine
int compare1(const void *a, const void *b)
{
	pairs *p = (pairs *)a;
	pairs *q = (pairs *)b;
	if (p->x > q->x)
		return 1;
	if (p->x == q->x)
		return 0;
	if (p->x < q->x)
		return -1;
}

void printSol(char *filename, double gBestTime)
{

	int i, j;
	FILE *fp = fopen(filename, "w");
	int cost, violations;
	finalfitness(gBestSolution, &cost, &violations);

	// write to file

	fprintf(fp, "{");

	for (i = 0; i < solcount; i++)
	{
		fprintf(fp, "(%d,%d,%d)", sol1[i].x + 1, sol1[i].y + 1, sol1[i].v);
		if (i < solcount - 1)
			fprintf(fp, ",");
	}

	fprintf(fp, "}");
	//fprintf(fp,"\nTime to best: %f",gBestTime);
	fclose(fp);
}
