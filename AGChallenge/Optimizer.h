#pragma once

#include "Evaluator.h"

#include "CIndividual.h"
#include "CTable.h"
#include "SmartPtr.h"

#include <algorithm>
#include <random>
#include <vector>

using namespace std;

class COptimizer
{
public:
	COptimizer(CEvaluator &cEvaluator);

	enum class SelectionMethod
	{
		Tournament,
		RandomTwo,
		Roulette
	};

	enum class CrossoverMethod
	{
		OnePoint,
		TwoPoint,
		Uniform
	};

	enum class MutationMethod
	{
		BitFlip,
		Swap,
		Scramble
	};

	void vInitialize();
	void vRunIteration();

	vector<int> *pvGetCurrentBest() { return &v_current_best; }

	// Optional GA parameter overrides
	void setPopulationSize(int n) { i_populationSize = n; }
	void setCrossoverProbability(double p) { d_crossoverProbability = p; }
	void setMutationProbability(double p) { d_mutationProbability = p; }
	void setSeed(unsigned int s) { c_rand_engine.seed(s); }
	void setSelectionMethod(SelectionMethod method) { e_selection_method = method; }
	void setCrossoverMethod(CrossoverMethod method) { e_crossover_method = method; }
	void setMutationMethod(MutationMethod method) { e_mutation_method = method; }
	void setElitismCount(int count) { i_elite_count = max(0, count); }
	void setInversionProbability(double p) { d_inversionProbability = max(0.0, min(1.0, p)); }
	void setUniformSwapProbability(double p) { d_uniform_swap_probability = max(0.0, min(1.0, p)); }

	// Counters
	long long getGenerationCount() const { return ll_generation_count; }
	long long getEvaluationCount() const { return ll_evaluation_count; }
	

private:
	void v_fill_randomly(vector<int> &vSolution);

	CEvaluator &c_evaluator;

	double d_current_best_fitness;
	vector<int> v_current_best;

	mt19937 c_rand_engine;
	long long ll_generation_count = 0;
	long long ll_evaluation_count = 0;

	SmartPtr<CTable<CIndividual*>> m_population;
	SmartPtr<CTable<CIndividual*>> newPopulation;

	int i_populationSize = 0;
	double d_crossoverProbability = 0;
	double d_mutationProbability = 0;
	double d_inversionProbability = 0;
	double d_uniform_swap_probability = 0.5;
	int i_elite_count = 0;

	SelectionMethod e_selection_method = SelectionMethod::Tournament;
	CrossoverMethod e_crossover_method = CrossoverMethod::OnePoint;
	MutationMethod e_mutation_method = MutationMethod::BitFlip;

	CIndividual* randomTwoSelection();
	CIndividual* tournamentSelection();
	CIndividual* rouletteSelection();
	CIndividual* selectParent();

	void applyCrossover(CIndividual &first, CIndividual &second);
	void applyMutation(CIndividual &individual);
	void applyInversion(CIndividual &individual);
	vector<CIndividual*> collectElites();

	void findBestIndividual();
	bool b_findBestIndividual();

	int getRandomNumber(int min, int max);
	double getRandomNumber(double min, double max);
	
	void printBestSolution();
};//class COptimizer
