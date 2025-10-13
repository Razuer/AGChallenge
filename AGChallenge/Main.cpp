#include "Evaluator.h"
#include "Optimizer.h"
#include "Timer.h"

#include <exception>
#include <iostream>
#include <random>

using namespace TimeCounters;

using namespace std;

// Default time limit in seconds (can be overridden via CLI)
static double g_time_limit_sec = 20 * 60;


void vRunExperiment(CEvaluator &cConfiguredEvaluator)
{
	try
	{
		CTimeCounter c_time_counter;

		double d_time_passed;

		COptimizer c_optimizer(cConfiguredEvaluator);

		c_time_counter.vSetStartNow();

		c_optimizer.vInitialize();

		c_time_counter.bGetTimePassed(&d_time_passed);

		while (d_time_passed <= g_time_limit_sec)
		{
			c_optimizer.vRunIteration();
			c_optimizer.pvGetCurrentBest();

			c_time_counter.bGetTimePassed(&d_time_passed);
		}//while (d_time_passed <= MAX_TIME)

		// Print final best fitness for quick comparisons
		if (auto pv_best = c_optimizer.pvGetCurrentBest())
		{
			double d_best = cConfiguredEvaluator.dEvaluate(*pv_best);
			cout << "fitness: " << d_best << endl;
		}

	}//try
	catch (exception &c_exception)
	{
		cout << c_exception.what() << endl;
	}//catch (exception &c_exception)
}//void vRunExperiment(const CEvaluator &cConfiguredEvaluator)

void vRunIsingSpinGlassExperiment(int iNumberOfBits, int iProblemSeed, int iMaskSeed)
{
	CIsingSpinGlassEvaluator c_ising_spin_glass;

	if (c_ising_spin_glass.bConfigure(iNumberOfBits, iProblemSeed, iMaskSeed) == true)
	{
		vRunExperiment(c_ising_spin_glass);
	}//if (c_ising_spin_glass.bConfigure(iNumberOfBits, iProblemSeed, iMaskSeed) == true)
}//void vRunIsingSpinGlassExperiment(int iNumberOfBits, int iProblemSeed, int iMaskSeed)

void vRunLeadingOnesExperiment(int iNumberOfBits, int iMaskSeed)
{
	CLeadingOnesEvaluator c_leading_ones;

	if (c_leading_ones.bConfigure(iNumberOfBits, iMaskSeed) == true)
	{
		vRunExperiment(c_leading_ones);
	}//if (c_leading_ones.bConfigure(iNumberOfBits, iMaskSeed) == true)
}//void vRunLeadingOnesExperiment(int iNumberOfBits, int iMaskSeed)

void vRunMaxSatExperiment(int iNumberOfBits, int iProblemSeed, float fClauseRatio, int iMaskSeed)
{
	CMaxSatEvaluator c_max_sat;

	if (c_max_sat.bConfigure(iNumberOfBits, iProblemSeed, fClauseRatio, iMaskSeed) == true)
	{
		vRunExperiment(c_max_sat);
	}//if (c_max_sat.bConfigure(iNumberOfBits, iProblemSeed, fClauseRatio, iMaskSeed) == true)
}//void vRunMaxSatExperiment(int iNumberOfBits, int iProblemSeed, float fClauseRatio, int iMaskSeed)

void vRunNearestNeighborNKExperiment(int iNumberOfBits, int iProblemSeed, int iK, int iMaskSeed)
{
	CNearestNeighborNKEvaluator c_nearest_neighbor_nk;

	if (c_nearest_neighbor_nk.bConfigure(iNumberOfBits, iProblemSeed, iK, iMaskSeed) == true)
	{
		vRunExperiment(c_nearest_neighbor_nk);
	}//if (c_nearest_neighbor_nk.bConfigure(iNumberOfBits, iProblemSeed, iK, iMaskSeed) == true)
}//void vRunNearestNeighborNKExperiment(int iNumberOfBits, int iProblemSeed, int iK, int iMaskSeed)

void vRunOneMaxExperiment(int iNumberOfBits, int iMaskSeed)
{
	COneMaxEvaluator c_one_max;

	if (c_one_max.bConfigure(iNumberOfBits, iMaskSeed) == true)
	{
		vRunExperiment(c_one_max);
	}//if (c_one_max.bConfigure(iNumberOfBits) == true)
}//void vRunOneMaxExperiment(int iNumberOfBits, int iMaskSeed)

void vRunRastriginExperiment(int iNumberOfBits, int iBitsPerFloat, int iMaskSeed)
{
	CRastriginEvaluator c_rastrigin;

	if (c_rastrigin.bConfigure(iNumberOfBits, iBitsPerFloat, iMaskSeed) == true)
	{
		vRunExperiment(c_rastrigin);
	}//if (c_rastrigin.bConfigure(iNumberOfBits, iBitsPerFloat, iMaskSeed) == true)
}//void vRunRastriginExperiment(int iNumberOfBits, int iBitsPerFloat, int iMaskSeed)

int main(int iArgCount, char **ppcArgValues)
{
	random_device c_mask_seed_generator;
	int i_mask_seed = (int)c_mask_seed_generator();

	// Quick-run CLI
	bool quick = false;
	for (int i = 1; i < iArgCount; ++i)
	{
		string arg = ppcArgValues[i];
		if (arg == "--quick")
		{
			quick = true;
		}
		else if (arg == "--quick-seconds" && i + 1 < iArgCount)
		{
			try { g_time_limit_sec = stod(ppcArgValues[++i]); }
			catch (...) { /* ignore parse errors, keep default */ }
		}
	}

#ifdef DEFAULT_QUICK_RUN
	quick = true;
#endif

	if (quick)
	{
		if (g_time_limit_sec == 20 * 60) g_time_limit_sec = 10; // default quick time
		// Single, fast experiment for quick iteration:
		vRunNearestNeighborNKExperiment(100, 0, 4, iSEED_NO_MASK);
	}
	else
	{
		vRunIsingSpinGlassExperiment(81, 0, i_mask_seed);
		vRunIsingSpinGlassExperiment(81, 0, iSEED_NO_MASK);

		vRunLeadingOnesExperiment(50, i_mask_seed);
		vRunLeadingOnesExperiment(50, iSEED_NO_MASK);

		vRunMaxSatExperiment(25, 0, 4.27f, i_mask_seed);
		vRunMaxSatExperiment(25, 0, 4.27f, iSEED_NO_MASK);

		vRunNearestNeighborNKExperiment(100, 0, 4, i_mask_seed);
		vRunNearestNeighborNKExperiment(100, 0, 4, iSEED_NO_MASK);

		vRunOneMaxExperiment(100, i_mask_seed);
		vRunOneMaxExperiment(100, iSEED_NO_MASK);

		vRunRastriginExperiment(200, 10, i_mask_seed);
		vRunRastriginExperiment(200, 10, iSEED_NO_MASK);
	}
	return 0;
}//int main(int iArgCount, char **ppcArgValues)