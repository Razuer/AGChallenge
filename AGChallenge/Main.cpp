#include "Evaluator.h"
#include "Optimizer.h"
#include "Timer.h"
#include "Knapsack.h"
#include "CIndividual.h"

#include <exception>
#include <iostream>
#include <random>
#include <fstream>
#include <filesystem>
#include <limits>

using namespace TimeCounters;

using namespace std;

// Default time limit in seconds (can be overridden via CLI)
static double g_time_limit_sec = 20 * 60;
// Optional GA parameter overrides (set via CLI)
static int g_pop = -1;
static double g_pc = -1;
static double g_pm = -1;
static int g_runs = 1;
// Problem selection
enum class Problem { NK, KNAP, ONEMAX, UNKNOWN };
static Problem g_problem = Problem::NK;
static string g_kp_instance;
static string g_kp_opt;
static string g_csv_path;
static unsigned int g_seed = 0; // 0 means random seed
static long long g_eval_budget = -1; // <=0 means disabled

static inline const char* problemToString(Problem p)
{
	switch (p)
	{
		case Problem::NK: return "nk";
		case Problem::KNAP: return "knap";
		case Problem::ONEMAX: return "onemax";
		default: return "unknown";
	}
}

// Runs the optimizer and returns final best fitness (NaN on error), also returning gens/evals
double dRunExperimentReturnFitnessAndStats(CEvaluator &cConfiguredEvaluator, long long &out_gens, long long &out_evals)
{
	try
	{
		CTimeCounter c_time_counter;
		double d_time_passed;

		COptimizer c_optimizer(cConfiguredEvaluator);
		// Apply CLI overrides before initialization
		if (g_pop > 0) c_optimizer.setPopulationSize(g_pop);
		if (g_pc >= 0) c_optimizer.setCrossoverProbability(g_pc);
		if (g_pm >= 0) c_optimizer.setMutationProbability(g_pm);
		if (g_seed != 0) {
			c_optimizer.setSeed(g_seed);
			CIndividual::seedRandom(g_seed);
		}

		// Initialize first, then start timer for fair comparisons
		c_optimizer.vInitialize();
		c_time_counter.vSetStartNow();
		c_time_counter.bGetTimePassed(&d_time_passed);

		// If evaluation budget is set and already met by initialization, skip iterations
		if (g_eval_budget > 0 && c_optimizer.getEvaluationCount() >= g_eval_budget)
		{
			out_gens = c_optimizer.getGenerationCount();
			out_evals = c_optimizer.getEvaluationCount();
			if (auto pv_best = c_optimizer.pvGetCurrentBest())
			{
				return cConfiguredEvaluator.dEvaluate(*pv_best);
			}
			return numeric_limits<double>::quiet_NaN();
		}

		while (d_time_passed <= g_time_limit_sec && (g_eval_budget <= 0 || c_optimizer.getEvaluationCount() < g_eval_budget))
		{
			c_optimizer.vRunIteration();
			c_optimizer.pvGetCurrentBest();
			c_time_counter.bGetTimePassed(&d_time_passed);
		}

		out_gens = c_optimizer.getGenerationCount();
		out_evals = c_optimizer.getEvaluationCount();
		if (auto pv_best = c_optimizer.pvGetCurrentBest())
		{
			return cConfiguredEvaluator.dEvaluate(*pv_best);
		}
		return numeric_limits<double>::quiet_NaN();
	}
	catch (exception &c_exception)
	{
		cout << c_exception.what() << endl;
		return numeric_limits<double>::quiet_NaN();
	}
}

// Backward-compatible wrapper ignoring stats
double dRunExperimentReturnFitness(CEvaluator &cConfiguredEvaluator)
{
	long long g = 0, e = 0;
	return dRunExperimentReturnFitnessAndStats(cConfiguredEvaluator, g, e);
}


void vRunExperiment(CEvaluator &cConfiguredEvaluator)
{
	double d_best = dRunExperimentReturnFitness(cConfiguredEvaluator);
	if (d_best == d_best) // not NaN
	{
		cout << "fitness: " << d_best << endl;
	}
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

	// Quick-run and GA CLI
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
		else if (arg == "--pop" && i + 1 < iArgCount)
		{
			try { g_pop = stoi(ppcArgValues[++i]); } catch (...) {}
		}
		else if (arg == "--pc" && i + 1 < iArgCount)
		{
			try { g_pc = stod(ppcArgValues[++i]); } catch (...) {}
		}
		else if (arg == "--pm" && i + 1 < iArgCount)
		{
			try { g_pm = stod(ppcArgValues[++i]); } catch (...) {}
		}
		else if (arg == "--runs" && i + 1 < iArgCount)
		{
			try { g_runs = stoi(ppcArgValues[++i]); } catch (...) {}
		}
		else if (arg == "--problem" && i + 1 < iArgCount)
		{
			string p = ppcArgValues[++i];
			if (p == "nk") g_problem = Problem::NK;
			else if (p == "knap") g_problem = Problem::KNAP;
			else if (p == "onemax") g_problem = Problem::ONEMAX;
		}
		else if (arg == "--kp-instance" && i + 1 < iArgCount)
		{
			g_kp_instance = ppcArgValues[++i];
		}
		else if (arg == "--kp-opt" && i + 1 < iArgCount)
		{
			g_kp_opt = ppcArgValues[++i];
		}
        else if (arg == "--csv" && i + 1 < iArgCount)
        {
            g_csv_path = ppcArgValues[++i];
        }
		else if (arg == "--seed" && i + 1 < iArgCount)
		{
			try { g_seed = static_cast<unsigned int>(stoul(ppcArgValues[++i])); } catch (...) { g_seed = 0; }
		}
		else if (arg == "--evals" && i + 1 < iArgCount)
		{
			try { g_eval_budget = static_cast<long long>(stoll(ppcArgValues[++i])); } catch (...) { g_eval_budget = -1; }
		}
	}

#ifdef DEFAULT_QUICK_RUN
	quick = true;
#endif

	if (quick)
	{
		if (g_time_limit_sec == 20 * 60) g_time_limit_sec = 10; // default quick time
		if (g_pop < 0) g_pop = 100; // smaller population for quick tests

		// Prepare CSV header if requested and file is new/empty
		if (!g_csv_path.empty())
		{
			bool write_header = true;
			std::error_code ec;
			if (std::filesystem::exists(g_csv_path, ec))
			{
				auto sz = std::filesystem::file_size(g_csv_path, ec);
				write_header = (ec ? true : (sz == 0));
			}
			std::ofstream csv(g_csv_path, ios::app);
			if (csv && write_header)
			{
				csv << "run,problem,pop,pc,pm,seconds,fitness,instance,generations,evaluations,seed" << '\n';
			}
		}

		for (int r = 0; r < g_runs; ++r)
		{
			switch (g_problem)
			{
				case Problem::KNAP:
				{
					// Defaults for quick knapsack if not provided
					if (g_kp_instance.empty()) {
#ifdef KP_INSTANCES_DIR
						g_kp_instance = string(KP_INSTANCES_DIR) + "low-dimensional/f1_l-d_kp_10_269";
#else
						g_kp_instance = "instances_01_KP/low-dimensional/f1_l-d_kp_10_269";
#endif
					}
					if (g_kp_opt.empty()) {
#ifdef KP_INSTANCES_DIR
						g_kp_opt = string(KP_INSTANCES_DIR) + "low-dimensional-optimum/f1_l-d_kp_10_269";
#else
						g_kp_opt = "instances_01_KP/low-dimensional-optimum/f1_l-d_kp_10_269";
#endif
					}

					CKnapSackEvaluator eval;
					double fit = numeric_limits<double>::quiet_NaN();
					long long gens = 0, evals = 0;
					if (eval.bConfigure(g_kp_instance, g_kp_opt))
					{
						fit = dRunExperimentReturnFitnessAndStats(eval, gens, evals);
						if (fit == fit) cout << "fitness: " << fit << endl;
					}
					if (!g_csv_path.empty())
					{
						std::ofstream csv(g_csv_path, ios::app);
						if (csv)
						{
							csv << (r + 1) << ','
							    << problemToString(g_problem) << ','
							    << (g_pop >= 0 ? to_string(g_pop) : string("")) << ','
							    << (g_pc >= 0 ? to_string(g_pc) : string("")) << ','
							    << (g_pm >= 0 ? to_string(g_pm) : string("")) << ','
							    << g_time_limit_sec << ','
							    << fit << ','
								<< g_kp_instance << ','
								<< gens << ','
								<< evals << ','
								<< (g_seed ? to_string(g_seed) : string(""))
							    << '\n';
						}
					}
					break;
				}
				case Problem::ONEMAX:
				{
					double fit = numeric_limits<double>::quiet_NaN();
					long long gens = 0, evals = 0;
					COneMaxEvaluator c_one_max;
					if (c_one_max.bConfigure(100, iSEED_NO_MASK))
					{
						fit = dRunExperimentReturnFitnessAndStats(c_one_max, gens, evals);
						if (fit == fit) cout << "fitness: " << fit << endl;
					}
					if (!g_csv_path.empty())
					{
						std::ofstream csv(g_csv_path, ios::app);
						if (csv)
						{
							csv << (r + 1) << ','
							    << problemToString(g_problem) << ','
							    << (g_pop >= 0 ? to_string(g_pop) : string("")) << ','
							    << (g_pc >= 0 ? to_string(g_pc) : string("")) << ','
							    << (g_pm >= 0 ? to_string(g_pm) : string("")) << ','
							    << g_time_limit_sec << ','
							    << fit << ','
								<< "-" << ','
								<< gens << ','
								<< evals << ','
								<< (g_seed ? to_string(g_seed) : string(""))
							    << '\n';
						}
					}
					break;
				}
				case Problem::NK:
				default:
				{
					double fit = numeric_limits<double>::quiet_NaN();
					long long gens = 0, evals = 0;
					{
						CNearestNeighborNKEvaluator eval;
						if (eval.bConfigure(100, 0, 4, iSEED_NO_MASK))
						{
							fit = dRunExperimentReturnFitnessAndStats(eval, gens, evals);
							if (fit == fit) cout << "fitness: " << fit << endl;
						}
					}
					if (!g_csv_path.empty())
					{
						std::ofstream csv(g_csv_path, ios::app);
						if (csv)
						{
							csv << (r + 1) << ','
							    << problemToString(g_problem) << ','
							    << (g_pop >= 0 ? to_string(g_pop) : string("")) << ','
							    << (g_pc >= 0 ? to_string(g_pc) : string("")) << ','
							    << (g_pm >= 0 ? to_string(g_pm) : string("")) << ','
							    << g_time_limit_sec << ','
							    << fit << ','
								<< "-" << ','
								<< gens << ','
								<< evals << ','
								<< (g_seed ? to_string(g_seed) : string(""))
							    << '\n';
						}
					}
					break;
				}
			}
		}
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