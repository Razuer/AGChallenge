#pragma once
#include <random>

#define BASIC_GEN_SIZE 5
#define iBIT_FALSE 0
#define iBIT_TRUE 1
#define UNKNOWN_FIT -1

using namespace std;

class CIndividual
{
private:
	vector<int> m_genotype;
	double m_fitness;
	bool m_updated;

	static mt19937& randomEngine();

public:
	// Constructor
	CIndividual(const vector<int>& genotype);
	CIndividual(int genotype_size);
	CIndividual(const CIndividual& pcOther);
	CIndividual();

	// Method for calculating the fitness of the individual
	double getFitness();
	void setFitness(double fit);
	bool isUpdated();
	void setUpdated(bool b);

	// Method for mutating the individual
	void tryMutate(const double& mutationProbability);
	void swapMutation();

	// Method for crossing the individual with another one and returning the children
	void crossover(CIndividual& other);
	void twoPointCrossover(CIndividual& other);

	// Getters
	vector<int> getGenotype() { return m_genotype; }
};