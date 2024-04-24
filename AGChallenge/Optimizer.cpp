#include "Optimizer.h"

#include <cfloat>
#include <iostream>
#include <windows.h>

using namespace std;

COptimizer::COptimizer(CEvaluator &cEvaluator)
	: c_evaluator(cEvaluator)
{
	random_device c_seed_generator;
	c_rand_engine.seed(c_seed_generator());

	d_current_best_fitness = 0;
}//COptimizer::COptimizer(CEvaluator &cEvaluator)


void COptimizer::vInitialize()
{
	d_current_best_fitness = -DBL_MAX;
	v_current_best.clear();


	// ------ Properties of Genetical Algorithm ------
	i_populationSize = 2000;
	d_crossoverProbability = 0.8;
	d_mutationProbability = 0.2;
	// -----------------------------------------------


	m_population = SmartPtr<CTable<CIndividual*>>(new CTable<CIndividual*>("population", i_populationSize));
	newPopulation = SmartPtr<CTable<CIndividual*>>(new CTable<CIndividual*>("newPopulation", i_populationSize));

	for (int i = 0; i < i_populationSize; i++)
	{
		vector<int> v_candidate;
		v_fill_randomly(v_candidate);
		m_population->vSetValueAt(i, new CIndividual((v_candidate)));
	}
	findBestIndividual();

}//void COptimizer::vInitialize()

void COptimizer::vRunIteration()
{
	// Create new population
	for (int j = 0; j < i_populationSize / 2; j++)
	{
		CIndividual* first = tournamentSelection();
		CIndividual* second = tournamentSelection();

		if (getRandomNumber(0.0, 1.0) < d_crossoverProbability) {
			first->twoPointCrossover(*second);
		}

		if (getRandomNumber(0.0, 1.0) < d_mutationProbability) {
			first->swapMutation();
		}
		if (getRandomNumber(0.0, 1.0) < d_mutationProbability) {
			second->swapMutation();
		}

		// Add two individuals to new population
		newPopulation->vSetValueAt(j * 2, first);
		newPopulation->vSetValueAt(j * 2 + 1, second);
	}

	// Compensate if the number of populations is odd
	if (i_populationSize & 1) {
		CIndividual* first = tournamentSelection();
		CIndividual* second = tournamentSelection();

		if (getRandomNumber(0.0, 1.0) < d_crossoverProbability) {
			first->twoPointCrossover(*second);
		}
		if (getRandomNumber(0.0, 1.0) < d_mutationProbability) {
			first->swapMutation();
		}
		
		newPopulation->vSetValueAt(i_populationSize - 1, first);
		delete second;
	}

	// Swap the old population with the new one, so main population will be new and old array wont be deleted
	m_population <<= newPopulation;
	findBestIndividual();

	// Delete newPopulation elements to prevent leaks
	for (int j = 0; j < i_populationSize; j++) {
		delete newPopulation->getValueAt(j);
	}
}//void COptimizer::vRunIteration()

void COptimizer::v_fill_randomly(vector<int> &vSolution)
{
	uniform_int_distribution<int> c_uniform_int_distribution(iBIT_FALSE, iBIT_TRUE);
	vSolution.resize((size_t)c_evaluator.iGetNumberOfBits());

	for (size_t i = 0; i < vSolution.size(); i++)
	{
		vSolution.at(i) = c_uniform_int_distribution(c_rand_engine);
	}//for (size_t i = 0; i < vSolution.size(); i++)
}//void COptimizer::v_fill_randomly(const vector<int> &vSolution)

// Method for selecting an individual using random two tournament selection
CIndividual* COptimizer::randomTwoSelection()
{
	CIndividual* first = m_population->getValueAt(getRandomNumber(0, i_populationSize - 1));
	CIndividual* second = m_population->getValueAt(getRandomNumber(0, i_populationSize - 1));

	if (first->getFitness() > second->getFitness()) {
		return new CIndividual(*first);
	}
	else return new CIndividual(*second);
}

CIndividual* COptimizer::tournamentSelection()
{
	int tournamentSize = 4;
	CIndividual* best = m_population->getValueAt(getRandomNumber(0, i_populationSize - 1));
	for (int i = 1; i < tournamentSize; i++)
	{
		CIndividual* candidate = m_population->getValueAt(getRandomNumber(0, i_populationSize - 1));
		if (candidate->getFitness() > best->getFitness())
			best = candidate;
	}
	return new CIndividual(*best);
}

// Method for getting the best solution found during the run of the genetic algorithm
void COptimizer::findBestIndividual()
{
	for (int i = 0; i < i_populationSize; i++)
	{
		CIndividual* current = m_population->getValueAt(i);
		if (!current->isUpdated()) {
			current->setFitness(c_evaluator.dEvaluate(&current->getGenotype()));
			current->setUpdated(true);
		}
		if (current->getFitness() > d_current_best_fitness) {
			d_current_best_fitness = current->getFitness();
			v_current_best = current->getGenotype();
		}
	}
}

// Method for getting the best solution found during the run of the genetic algorithm with boolean
bool COptimizer::b_findBestIndividual()
{
	bool changed = false;
	for (int i = 0; i < i_populationSize; i++)
	{
		CIndividual* current = m_population->getValueAt(i);
		if (!current->isUpdated()) {
			current->setFitness(c_evaluator.dEvaluate(&current->getGenotype()));
			current->setUpdated(true);
		}
		if (current->getFitness() > d_current_best_fitness) {
			d_current_best_fitness = current->getFitness();
			v_current_best = current->getGenotype();
			changed = true;
		}
	}
	return changed;
}

// Methods for generating random numbers
int COptimizer::getRandomNumber(int min, int max) {
	uniform_int_distribution<int> dist(min, max);
	return dist(c_rand_engine);
}

double COptimizer::getRandomNumber(double min, double max) {
	uniform_real_distribution<double> dist(min, max);
	return dist(c_rand_engine);
}

void COptimizer::printBestSolution() {
	cout << "fitness: " << d_current_best_fitness << endl;
	/*cout << "genotype: ";
	for (int i : v_current_best)
	{
		cout << i << " ";
	}*/
}