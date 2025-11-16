#include "Optimizer.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>

using namespace std;

COptimizer::COptimizer(CEvaluator &cEvaluator)
	: c_evaluator(cEvaluator)
{
	random_device c_seed_generator;
	c_rand_engine.seed(c_seed_generator());

	d_current_best_fitness = 0;
    ll_generation_count = 0;
    ll_evaluation_count = 0;
}//COptimizer::COptimizer(CEvaluator &cEvaluator)


void COptimizer::vInitialize()
{
	d_current_best_fitness = -DBL_MAX;
	v_current_best.clear();
    ll_generation_count = 0;
    ll_evaluation_count = 0;


	// ------ Properties of Genetical Algorithm ------
	if (i_populationSize <= 0) i_populationSize = 2000;
	if (d_crossoverProbability <= 0) d_crossoverProbability = 0.8;
	if (d_mutationProbability <= 0) d_mutationProbability = 0.2;
	// -----------------------------------------------

	if (i_elite_count > i_populationSize) {
		i_elite_count = i_populationSize;
	}
	if (d_uniform_swap_probability < 0.0) {
		d_uniform_swap_probability = 0.0;
	}
	if (d_uniform_swap_probability > 1.0) {
		d_uniform_swap_probability = 1.0;
	}

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
	findBestIndividual(); // ensure fitnesses are available for selection

	vector<CIndividual*> elites = collectElites();
	int fill_index = 0;

	for (CIndividual* elite : elites)
	{
		newPopulation->vSetValueAt(fill_index++, elite);
	}

	while (fill_index < i_populationSize)
	{
		CIndividual* first = selectParent();
		CIndividual* second = selectParent();

		if (getRandomNumber(0.0, 1.0) < d_crossoverProbability) {
			applyCrossover(*first, *second);
		}

		applyMutation(*first);
		applyMutation(*second);
		applyInversion(*first);
		applyInversion(*second);

		newPopulation->vSetValueAt(fill_index++, first);
		if (fill_index < i_populationSize) {
			newPopulation->vSetValueAt(fill_index++, second);
		}
		else {
			delete second;
		}
	}

	// Swap the old population with the new one, so main population will be new and old array wont be deleted
	m_population <<= newPopulation;
	findBestIndividual();
	ll_generation_count++;

	// Delete newPopulation elements to prevent leaks
	for (int j = 0; j < i_populationSize; j++) {
		delete newPopulation->getValueAt(j);
		newPopulation->vSetValueAt(j, nullptr);
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

CIndividual* COptimizer::rouletteSelection()
{
	vector<double> cumulative;
	cumulative.reserve(static_cast<size_t>(i_populationSize));

	double min_fitness = numeric_limits<double>::infinity();

	for (int i = 0; i < i_populationSize; i++)
	{
		CIndividual* current = m_population->getValueAt(i);
		if (!current->isUpdated()) {
			current->setFitness(c_evaluator.dEvaluate(current->getGenotype()));
			ll_evaluation_count++;
			current->setUpdated(true);
		}
		min_fitness = min(min_fitness, current->getFitness());
	}

	double offset = 0.0;
	if (!std::isfinite(min_fitness)) {
		min_fitness = 0.0;
	}
	if (min_fitness < 0.0) {
		offset = -min_fitness + 1e-9;
	}

	double total = 0.0;
	for (int i = 0; i < i_populationSize; i++)
	{
		double weight = m_population->getValueAt(i)->getFitness() + offset;
		if (weight < 0.0) {
			weight = 0.0;
		}
		total += weight;
		cumulative.push_back(total);
	}

	if (total <= 0.0) {
		return tournamentSelection();
	}

	double sample = getRandomNumber(0.0, total);
	auto it = lower_bound(cumulative.begin(), cumulative.end(), sample);
	size_t index = it == cumulative.end() ? cumulative.size() - 1 : static_cast<size_t>(distance(cumulative.begin(), it));

	return new CIndividual(*m_population->getValueAt(static_cast<int>(index)));
}

CIndividual* COptimizer::selectParent()
{
	switch (e_selection_method)
	{
	case SelectionMethod::RandomTwo:
		return randomTwoSelection();
	case SelectionMethod::Roulette:
		return rouletteSelection();
	case SelectionMethod::Tournament:
	default:
		return tournamentSelection();
	}
}

void COptimizer::applyCrossover(CIndividual &first, CIndividual &second)
{
	switch (e_crossover_method)
	{
	case CrossoverMethod::TwoPoint:
		first.twoPointCrossover(second);
		break;
	case CrossoverMethod::Uniform:
		first.uniformCrossover(second, d_uniform_swap_probability);
		break;
	case CrossoverMethod::OnePoint:
	default:
		first.crossover(second);
		break;
	}
}

void COptimizer::applyMutation(CIndividual &individual)
{
	switch (e_mutation_method)
	{
	case MutationMethod::BitFlip:
		individual.tryMutate(d_mutationProbability);
		break;
	case MutationMethod::Swap:
		if (getRandomNumber(0.0, 1.0) < d_mutationProbability) {
			individual.swapMutation();
		}
		break;
	case MutationMethod::Scramble:
		if (getRandomNumber(0.0, 1.0) < d_mutationProbability) {
			individual.scrambleMutation();
		}
		break;
	default:
		break;
	}
}

void COptimizer::applyInversion(CIndividual &individual)
{
	if (d_inversionProbability > 0.0 && getRandomNumber(0.0, 1.0) < d_inversionProbability) {
		individual.inversionMutation();
	}
}

vector<CIndividual*> COptimizer::collectElites()
{
	int eliteCount = min(i_elite_count, i_populationSize);
	vector<CIndividual*> elites;
	if (eliteCount <= 0) {
		return elites;
	}

	vector<int> indices(static_cast<size_t>(i_populationSize));
	iota(indices.begin(), indices.end(), 0);
	sort(indices.begin(), indices.end(), [&](int lhs, int rhs) {
		return m_population->getValueAt(lhs)->getFitness() > m_population->getValueAt(rhs)->getFitness();
	});

	elites.reserve(static_cast<size_t>(eliteCount));
	for (int i = 0; i < eliteCount; i++)
	{
		CIndividual* source = m_population->getValueAt(indices[static_cast<size_t>(i)]);
		elites.push_back(new CIndividual(*source));
	}
	return elites;
}

// Method for getting the best solution found during the run of the genetic algorithm
void COptimizer::findBestIndividual()
{
	for (int i = 0; i < i_populationSize; i++)
	{
		CIndividual* current = m_population->getValueAt(i);
		if (!current->isUpdated()) {
			current->setFitness(c_evaluator.dEvaluate(current->getGenotype()));
			ll_evaluation_count++;
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
			current->setFitness(c_evaluator.dEvaluate(current->getGenotype()));
			ll_evaluation_count++;
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
