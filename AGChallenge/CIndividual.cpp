#include "CIndividual.h"

	// Constructor
CIndividual::CIndividual(const vector<int>& genotype)
{
	m_genotype = genotype;
	m_updated = false;
	m_fitness = UNKNOWN_FIT;
}

CIndividual::CIndividual(int genotype_size)
{
	uniform_int_distribution<int> dist(iBIT_FALSE, iBIT_TRUE);
	m_genotype.reserve(genotype_size);
	for (int i = 0; i < genotype_size; i++)
	{
		m_genotype.push_back(dist(randomEngine()));
	}
	m_updated = false;
	m_fitness = UNKNOWN_FIT;
}

CIndividual::CIndividual(const CIndividual& pcOther) {
	m_genotype = pcOther.m_genotype;
	m_fitness = pcOther.m_fitness;
}

CIndividual::CIndividual() {
	m_fitness = UNKNOWN_FIT;
	m_updated = false;
}

double CIndividual::getFitness()
{
	return m_fitness;
}

void CIndividual::setFitness(double fit) {
	m_fitness = fit;
}

bool CIndividual::isUpdated() {
	return m_updated;
}

void CIndividual::setUpdated(bool b) {
	m_updated = b;
}

mt19937& CIndividual::randomEngine() {
	static mt19937 engine(random_device{}());
	return engine;
}

// Method for mutating the individual
void CIndividual::tryMutate(const double& mutationProbability)
{
	uniform_real_distribution<double> dist(0.0, 1.0);
	
	bool mutated = false;
	for (int i = 0; i < m_genotype.size(); i++) {
		if (dist(randomEngine()) < mutationProbability) {
			m_genotype[i] ^= 1;
			mutated = true;
		}
	}
	if (mutated) {
		m_updated = false;
	}
}

// 2nd more efficient (i guess) method for mutating the individual
void CIndividual::swapMutation()
{
	uniform_int_distribution<int> dist(0, m_genotype.size() - 1);
	int mutationPoint1 = dist(randomEngine());
	int mutationPoint2 = dist(randomEngine());

	swap(m_genotype[mutationPoint1], m_genotype[mutationPoint2]);
	m_updated = false;
	m_fitness = UNKNOWN_FIT;
}


// Method for crossing the individual with another one
void CIndividual::crossover(CIndividual& other)
{
	uniform_int_distribution<int> dist(1, m_genotype.size() - 1);
	int crossoverPoint = dist(randomEngine());

	for (int i = crossoverPoint; i < m_genotype.size(); i++) {
		swap(m_genotype[i], other.m_genotype[i]);
	}
	m_updated = false;
	m_fitness = UNKNOWN_FIT;
	other.m_updated = false;
	other.m_fitness = UNKNOWN_FIT;
}

void CIndividual::twoPointCrossover(CIndividual& other)
{
	uniform_int_distribution<int> dist(1, m_genotype.size() - 1);
	int crossoverPoint1 = dist(randomEngine());
	int crossoverPoint2 = dist(randomEngine());

	if (crossoverPoint2 < crossoverPoint1)
		swap(crossoverPoint1, crossoverPoint2);

	for (int i = crossoverPoint1; i < crossoverPoint2; i++)
	{
		swap(m_genotype[i], other.m_genotype[i]);
	}
	m_updated = false;
	m_fitness = UNKNOWN_FIT;
	other.m_updated = false;
	other.m_fitness = UNKNOWN_FIT;
}
