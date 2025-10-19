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
// Batch mode
static string g_batch_path; // path to batch config file; when set, batch mode runs

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
		else if (arg == "--batch" && i + 1 < iArgCount)
		{
			g_batch_path = ppcArgValues[++i];
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

	// If batch mode is requested, handle it and exit.
	if (!g_batch_path.empty())
	{
		// Minimal parsing helpers
		auto ltrim = [](string &s){ s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch){ return !std::isspace(ch); })); };
		auto rtrim = [](string &s){ s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), s.end()); };
		auto trim = [&](string &s){ ltrim(s); rtrim(s); };
		auto split_csv = [](const string &line){
			vector<string> cols; string cur; bool in_quote=false; for (size_t i=0;i<line.size();++i){ char c=line[i]; if(c=='"'){ in_quote=!in_quote; }
				else if(c==',' && !in_quote){ cols.push_back(cur); cur.clear(); } else { cur.push_back(c);} }
			cols.push_back(cur); return cols; };

		// Read file
		ifstream in(g_batch_path);
		if (!in)
		{
			cerr << "Failed to open batch file: " << g_batch_path << endl;
			return 1;
		}

		string instance_path;
		string optimum_path;
		vector<string> lines;
		string line;
		while (std::getline(in, line)) { lines.push_back(line); }

		// Remove comments and trim
		for (auto &ln : lines)
		{
			auto pos_hash = ln.find('#');
			if (pos_hash != string::npos) ln = ln.substr(0, pos_hash);
			trim(ln);
		}

		// Extract key=value pairs for instance/optimum from top of file
		size_t idx = 0;
		for (; idx < lines.size(); ++idx)
		{
			if (lines[idx].empty()) continue;
			auto pos_eq = lines[idx].find('=');
			if (pos_eq == string::npos) break;
			string key = lines[idx].substr(0, pos_eq);
			string val = lines[idx].substr(pos_eq + 1);
			trim(key); trim(val);
			if (key == "instance") instance_path = val;
			else if (key == "optimum") optimum_path = val;
			else break; // stop on unknown key
		}

		// Next non-empty line should be either a CSV header for configs, or a CSV with instance,optimum,... per-row
		while (idx < lines.size() && lines[idx].empty()) ++idx;
		if (idx >= lines.size())
		{
			cerr << "Batch file contains no configurations: " << g_batch_path << endl;
			return 1;
		}

		// Prepare CSV output
		if (g_csv_path.empty())
		{
			// derive output path from batch path: <stem>_results.csv in the same directory
			std::filesystem::path bp(g_batch_path);
			std::string stem = bp.stem().string();
			std::filesystem::path out = bp.parent_path() / (stem + std::string("_results.csv"));
			g_csv_path = out.string();
		}
		bool write_header = true;
		{
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

		// Determine header and parse rows
		string header = lines[idx];
		vector<string> header_cols = split_csv(header);
		for (auto &h : header_cols) trim(h);
		bool per_row_has_instance = false;
		size_t col_offset = 0;
		if (!header_cols.empty() && header_cols[0] == "instance")
		{
			per_row_has_instance = true;
			col_offset = 2; // instance,optimum
		}
		else
		{
			// Require instance_path to be provided above
			if (instance_path.empty())
			{
				cerr << "Batch file: provide global 'instance=...' (and optional 'optimum=...') or include instance,optimum in header." << endl;
				return 1;
			}
		}

		auto get_col_index = [&](const string &name)->int{
			for (size_t i = 0; i < header_cols.size(); ++i) if (header_cols[i] == name) return (int)i;
			return -1;
		};

		int idx_instance = per_row_has_instance ? get_col_index("instance") : -1;
		int idx_optimum  = per_row_has_instance ? get_col_index("optimum")  : -1;
		int idx_pop = get_col_index("pop");
		int idx_pc  = get_col_index("pc");
		int idx_pm  = get_col_index("pm");
		int idx_seconds = get_col_index("seconds");
		int idx_runs = get_col_index("runs");
		int idx_seed = get_col_index("seed");
		int idx_evals = get_col_index("evals"); // optional evaluation budget per config

		// Validate required columns
		if (idx_pop < 0 || idx_pc < 0 || idx_pm < 0 || idx_seconds < 0)
		{
			cerr << "Batch header must contain at least: pop,pc,pm,seconds and optionally runs,seed,evals" << endl;
			return 1;
		}

		// Iterate data rows
		long long batch_run_counter = 0;
		for (size_t rline = idx + 1; rline < lines.size(); ++rline)
		{
			if (lines[rline].empty()) continue;
			vector<string> cols = split_csv(lines[rline]);
			// pad columns
			if (cols.size() < header_cols.size()) cols.resize(header_cols.size());
			for (auto &c : cols) { string t=c; trim(t); c=t; }

			string inst = per_row_has_instance && idx_instance >= 0 ? cols[(size_t)idx_instance] : instance_path;
			string opt  = per_row_has_instance && idx_optimum >= 0  ? cols[(size_t)idx_optimum]  : optimum_path;

			// Configure GA params
			int pop = stoi(cols[(size_t)idx_pop]);
			double pc = stod(cols[(size_t)idx_pc]);
			double pm = stod(cols[(size_t)idx_pm]);
			double seconds = stod(cols[(size_t)idx_seconds]);
			int runs = (idx_runs >= 0 && !cols[(size_t)idx_runs].empty()) ? stoi(cols[(size_t)idx_runs]) : 1;
			unsigned int seed = (idx_seed >= 0 && !cols[(size_t)idx_seed].empty()) ? (unsigned int)stoul(cols[(size_t)idx_seed]) : 0u;
			long long evals_budget = (idx_evals >= 0 && !cols[(size_t)idx_evals].empty()) ? stoll(cols[(size_t)idx_evals]) : -1;

			// Set globals for this config
			g_problem = Problem::KNAP;
			g_pop = pop; g_pc = pc; g_pm = pm; g_time_limit_sec = seconds; g_runs = runs; g_seed = seed; g_eval_budget = evals_budget;

			for (int rr = 0; rr < runs; ++rr)
			{
				CKnapSackEvaluator eval;
				double fit = numeric_limits<double>::quiet_NaN();
				long long gens = 0, evals = 0;
				if (eval.bConfigure(inst, opt))
				{
					fit = dRunExperimentReturnFitnessAndStats(eval, gens, evals);
					if (fit == fit) cout << "fitness: " << fit << endl;
				}
				std::ofstream csv(g_csv_path, ios::app);
				if (csv)
				{
					csv << (++batch_run_counter) << ','
						<< problemToString(g_problem) << ','
						<< g_pop << ','
						<< g_pc << ','
						<< g_pm << ','
						<< g_time_limit_sec << ','
						<< fit << ','
						<< inst << ','
						<< gens << ','
						<< evals << ','
						<< (g_seed ? to_string(g_seed) : string(""))
						<< '\n';
				}
			}
		}

		return 0;
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