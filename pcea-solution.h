#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <limits.h>

#define INF INT_MAX
#define NRUNS 10
#define POPSIZE 40 // population size
#define XOVER 1
#define MUTATION 0
#define XPROB 60
#define STOREBIAS 50 // % bias toward store Xover and Mutation
#define MAX_IMPROVE_LIMIT 10000

/* Global Declarations */

FILE *fp;

typedef struct s_tag
{
	int x, y, v;
} pairs;

int *gBestSolution;	 // Global Best Solution
int gBestFitness;	 // Global Best Fitness
int gBestViolations; // Global Best Violations
//int gBestCount=-1; // Global Best Fitness
int *pop;				 // Population
int popFitness[POPSIZE]; // Fitness of each chromosome of the evolving population
int violations[POPSIZE];
int *o1, *o2;	 // Offspring after mutation (1) and crossover (1,2) respectively
int totalDemand; // store the sum of all store demands in this variable
int warehouses, stores, incompatibilities;
char ch;
int *capacities, *fixedcosts, *goods;
int *supplycosts;
pairs *incompatiblepairs; //, *supplycosts1;
pairs *sol1;
int *sol;
int solcount;
int *fcapacities, *demand;
int *openwarehouses;
int *p1, *p2;

/* Function declarations */
void fitness(int[], int *, int *);
int compare1(const void *, const void *);
unsigned ComputeCost();
void printSol(char *, double);
int binarySearch(pairs[], int, int, int);
void readData(char *);
int notcorrectgbest(void);
void freepop(void);
void displaypop(void);
void initpop(void);
void reinit(void);
int findWH(int, int *, int[]);
void fitness(int[], int *, int *);
void finalfitness(int[], int *, int *);
void crossover_order(int *, int *, int);
void crossover_alternation(int *, int *, int);
void mutation_swap(int *, int);
int binarySearch(pairs[], int, int, int);
unsigned ComputeCost(void);
int _2_T(void);
int _3_T(void);
void display(int *, int);
int ealoop(void);
int compare1(const void *, const void *);
void printSol(char *, double);

/* MR */
void test();
void readCapacity(int *capacities_mr, int ware_houses);

#endif
