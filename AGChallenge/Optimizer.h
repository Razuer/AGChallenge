#pragma once

#include "Evaluator.h"

#include "CIndividual.h"
#include "CTable.h"
#include "SmartPtr.h"

#include <random>
#include <vector>

using namespace std;

class COptimizer
{
public:
	COptimizer(CEvaluator &cEvaluator);

	void vInitialize();
	void vRunIteration();

	vector<int> *pvGetCurrentBest() { return &v_current_best; }
	

private:
	void v_fill_randomly(vector<int> &vSolution);

	CEvaluator &c_evaluator;

	double d_current_best_fitness;
	vector<int> v_current_best;

	mt19937 c_rand_engine;

	SmartPtr<CTable<CIndividual*>> m_population;
	SmartPtr<CTable<CIndividual*>> newPopulation;

	int i_populationSize = 0;
	double d_crossoverProbability = 0;
	double d_mutationProbability = 0;

	CIndividual* randomTwoSelection();
	CIndividual* tournamentSelection();

	void findBestIndividual();
	bool b_findBestIndividual();

	int getRandomNumber(int min, int max);
	double getRandomNumber(double min, double max);
	
	void printBestSolution();
};//class COptimizer