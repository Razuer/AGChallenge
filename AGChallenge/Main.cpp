#include "Evaluator.h"
#include "Optimizer.h"
#include "Timer.h"
#include "Knapsack.h"
#include "CIndividual.h"

#include <algorithm>
#include <cctype>
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
// Operator configuration (defaults replicate Zad1 behaviour)
static COptimizer::SelectionMethod g_selection_method = COptimizer::SelectionMethod::Tournament;
static COptimizer::CrossoverMethod g_crossover_method = COptimizer::CrossoverMethod::OnePoint;
static COptimizer::MutationMethod g_mutation_method = COptimizer::MutationMethod::BitFlip;
static int g_elite_count = 0;
static double g_inversion_probability = 0.0;
static double g_uniform_swap_probability = 0.5;

static constexpr const char* CSV_HEADER = "run,problem,pop,pc,pm,seconds,fitness,instance,generations,evaluations,seed,selection,crossover,mutation,elitism,pinv,uniform_swap";

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

static string normalizeToken(string value)
{
	transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char c) { return c == '-' || c == '_' || std::isspace(c); }), value.end());
	return value;
}

static bool parseSelectionMethod(const string &value, COptimizer::SelectionMethod &out)
{
	string token = normalizeToken(value);
	if (token == "tournament") {
		out = COptimizer::SelectionMethod::Tournament;
		return true;
	}
	if (token == "random2" || token == "randomtwo") {
		out = COptimizer::SelectionMethod::RandomTwo;
		return true;
	}
	if (token == "roulette") {
		out = COptimizer::SelectionMethod::Roulette;
		return true;
	}
	return false;
}

static bool parseCrossoverMethod(const string &value, COptimizer::CrossoverMethod &out)
{
	string token = normalizeToken(value);
	if (token == "onepoint" || token == "singlepoint") {
		out = COptimizer::CrossoverMethod::OnePoint;
		return true;
	}
	if (token == "twopoint" || token == "doublepoint") {
		out = COptimizer::CrossoverMethod::TwoPoint;
		return true;
	}
	if (token == "uniform") {
		out = COptimizer::CrossoverMethod::Uniform;
		return true;
	}
	return false;
}

static bool parseMutationMethod(const string &value, COptimizer::MutationMethod &out)
{
	string token = normalizeToken(value);
	if (token == "bitflip" || token == "flip") {
		out = COptimizer::MutationMethod::BitFlip;
		return true;
	}
	if (token == "swap") {
		out = COptimizer::MutationMethod::Swap;
		return true;
	}
	if (token == "scramble") {
		out = COptimizer::MutationMethod::Scramble;
		return true;
	}
	return false;
}

static string selectionToString(COptimizer::SelectionMethod method)
{
	switch (method)
	{
	case COptimizer::SelectionMethod::RandomTwo: return "random-two";
	case COptimizer::SelectionMethod::Roulette: return "roulette";
	case COptimizer::SelectionMethod::Tournament:
	default: return "tournament";
	}
}

static string crossoverToString(COptimizer::CrossoverMethod method)
{
	switch (method)
	{
	case COptimizer::CrossoverMethod::TwoPoint: return "two-point";
	case COptimizer::CrossoverMethod::Uniform: return "uniform";
	case COptimizer::CrossoverMethod::OnePoint:
	default: return "one-point";
	}
}

static string mutationToString(COptimizer::MutationMethod method)
{
	switch (method)
	{
	case COptimizer::MutationMethod::Swap: return "swap";
	case COptimizer::MutationMethod::Scramble: return "scramble";
	case COptimizer::MutationMethod::BitFlip:
	default: return "bit-flip";
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
		c_optimizer.setSelectionMethod(g_selection_method);
		c_optimizer.setCrossoverMethod(g_crossover_method);
		c_optimizer.setMutationMethod(g_mutation_method);
		c_optimizer.setElitismCount(g_elite_count);
		c_optimizer.setInversionProbability(g_inversion_probability);
		c_optimizer.setUniformSwapProbability(g_uniform_swap_probability);
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
		else if (arg == "--selection" && i + 1 < iArgCount)
		{
			string value = ppcArgValues[++i];
			if (!parseSelectionMethod(value, g_selection_method))
			{
				cerr << "Unknown selection method: " << value << endl;
				return 1;
			}
		}
		else if (arg == "--crossover-method" && i + 1 < iArgCount)
		{
			string value = ppcArgValues[++i];
			if (!parseCrossoverMethod(value, g_crossover_method))
			{
				cerr << "Unknown crossover method: " << value << endl;
				return 1;
			}
		}
		else if (arg == "--mutation-method" && i + 1 < iArgCount)
		{
			string value = ppcArgValues[++i];
			if (!parseMutationMethod(value, g_mutation_method))
			{
				cerr << "Unknown mutation method: " << value << endl;
				return 1;
			}
		}
		else if (arg == "--elitism" && i + 1 < iArgCount)
		{
			try { g_elite_count = std::max(0, stoi(ppcArgValues[++i])); } catch (...) { g_elite_count = 0; }
		}
		else if (arg == "--pinv" && i + 1 < iArgCount)
		{
			try { g_inversion_probability = stod(ppcArgValues[++i]); } catch (...) { g_inversion_probability = 0.0; }
		}
		else if (arg == "--uniform-swap" && i + 1 < iArgCount)
		{
			try { g_uniform_swap_probability = stod(ppcArgValues[++i]); } catch (...) { g_uniform_swap_probability = 0.5; }
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

		// Extract key=value pairs for instance/optimum and optional defaults from top of file
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
			else if (key == "selection") {
				if (!parseSelectionMethod(val, g_selection_method))
				{
					cerr << "Batch file: unknown selection method '" << val << "'" << endl;
					return 1;
				}
			}
			else if (key == "crossover" || key == "crossover-method") {
				if (!parseCrossoverMethod(val, g_crossover_method))
				{
					cerr << "Batch file: unknown crossover method '" << val << "'" << endl;
					return 1;
				}
			}
			else if (key == "mutation" || key == "mutation-method") {
				if (!parseMutationMethod(val, g_mutation_method))
				{
					cerr << "Batch file: unknown mutation method '" << val << "'" << endl;
					return 1;
				}
			}
			else if (key == "elitism") {
				try { g_elite_count = std::max(0, stoi(val)); }
				catch (...) {
					cerr << "Batch file: invalid elitism value '" << val << "'" << endl;
					return 1;
				}
			}
			else if (key == "pinv") {
				try { g_inversion_probability = std::clamp(stod(val), 0.0, 1.0); }
				catch (...) {
					cerr << "Batch file: invalid pinv value '" << val << "'" << endl;
					return 1;
				}
			}
			else if (key == "uniform_swap" || key == "uniform-swap") {
				try { g_uniform_swap_probability = std::clamp(stod(val), 0.0, 1.0); }
				catch (...) {
					cerr << "Batch file: invalid uniform_swap value '" << val << "'" << endl;
					return 1;
				}
			}
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
		bool derived_default_csv = false;
		if (g_csv_path.empty())
		{
			// derive output path from batch path: <stem>_results.csv in the same directory
			std::filesystem::path bp(g_batch_path);
			std::string stem = bp.stem().string();
			std::filesystem::path out = bp.parent_path() / (stem + std::string("_results.csv"));
			g_csv_path = out.string();
			derived_default_csv = true; // when auto-generating _results.csv, overwrite on new run
		}

		// Open header with truncation if we auto-derived the default file, otherwise append
		{
			std::ofstream csv(g_csv_path, derived_default_csv ? (ios::out | ios::trunc) : (ios::out | ios::app));
			if (csv)
			{
				// If truncating, the file is empty so we always (re)write the header.
				// If appending to a user-provided path, write header only if the file was empty.
				bool should_write_header = true;
				if (!derived_default_csv)
				{
					std::error_code ec;
					auto sz = std::filesystem::file_size(g_csv_path, ec);
					should_write_header = (ec ? true : (sz == 0));
				}
				if (should_write_header)
				{
					csv << CSV_HEADER << '\n';
				}
			}
		}

		// Determine header and parse rows
		string header = lines[idx];
		vector<string> header_cols = split_csv(header);
		for (auto &h : header_cols) trim(h);
		bool per_row_has_instance = false;
		if (!header_cols.empty() && header_cols[0] == "instance")
		{
			per_row_has_instance = true;
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
		int idx_selection = get_col_index("selection");
		int idx_crossover = get_col_index("crossover-method");
		if (idx_crossover < 0) idx_crossover = get_col_index("crossover");
		int idx_mutation = get_col_index("mutation-method");
		if (idx_mutation < 0) idx_mutation = get_col_index("mutation");
		int idx_elitism = get_col_index("elitism");
		int idx_pinv = get_col_index("pinv");
		int idx_uniform = get_col_index("uniform-swap");
		if (idx_uniform < 0) idx_uniform = get_col_index("uniform_swap");
		if (idx_uniform < 0) idx_uniform = get_col_index("uniformswap");

		// Validate required columns
		if (idx_pop < 0 || idx_pc < 0 || idx_pm < 0 || idx_seconds < 0)
		{
			cerr << "Batch header must contain at least: pop,pc,pm,seconds and optionally runs,seed,evals" << endl;
			return 1;
		}

		// Defaults for optional GA settings per-row
		COptimizer::SelectionMethod selection_default = g_selection_method;
		COptimizer::CrossoverMethod crossover_default = g_crossover_method;
		COptimizer::MutationMethod mutation_default = g_mutation_method;
		int elitism_default = g_elite_count;
		double pinv_default = g_inversion_probability;
		double uniform_default = g_uniform_swap_probability;

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
			g_selection_method = selection_default;
			g_crossover_method = crossover_default;
			g_mutation_method = mutation_default;
			g_elite_count = elitism_default;
			g_inversion_probability = pinv_default;
			g_uniform_swap_probability = uniform_default;

			auto reportError = [&](const string &field, const string &value){
				cerr << "Batch file (" << g_batch_path << "), line " << (rline + 1) << ": invalid " << field << " value '" << value << "'" << endl;
				return 1;
			};

			if (idx_selection >= 0 && !cols[(size_t)idx_selection].empty())
			{
				if (!parseSelectionMethod(cols[(size_t)idx_selection], g_selection_method))
				{
					return reportError("selection", cols[(size_t)idx_selection]);
				}
			}
			if (idx_crossover >= 0 && !cols[(size_t)idx_crossover].empty())
			{
				if (!parseCrossoverMethod(cols[(size_t)idx_crossover], g_crossover_method))
				{
					return reportError("crossover", cols[(size_t)idx_crossover]);
				}
			}
			if (idx_mutation >= 0 && !cols[(size_t)idx_mutation].empty())
			{
				if (!parseMutationMethod(cols[(size_t)idx_mutation], g_mutation_method))
				{
					return reportError("mutation", cols[(size_t)idx_mutation]);
				}
			}
			if (idx_elitism >= 0 && !cols[(size_t)idx_elitism].empty())
			{
				try { g_elite_count = std::max(0, stoi(cols[(size_t)idx_elitism])); }
				catch (...) { return reportError("elitism", cols[(size_t)idx_elitism]); }
			}
			if (idx_pinv >= 0 && !cols[(size_t)idx_pinv].empty())
			{
				try { g_inversion_probability = std::clamp(stod(cols[(size_t)idx_pinv]), 0.0, 1.0); }
				catch (...) { return reportError("pinv", cols[(size_t)idx_pinv]); }
			}
			if (idx_uniform >= 0 && !cols[(size_t)idx_uniform].empty())
			{
				try { g_uniform_swap_probability = std::clamp(stod(cols[(size_t)idx_uniform]), 0.0, 1.0); }
				catch (...) { return reportError("uniform_swap", cols[(size_t)idx_uniform]); }
			}

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
						<< (g_seed ? to_string(g_seed) : string("")) << ','
						<< selectionToString(g_selection_method) << ','
						<< crossoverToString(g_crossover_method) << ','
						<< mutationToString(g_mutation_method) << ','
						<< g_elite_count << ','
						<< g_inversion_probability << ','
						<< g_uniform_swap_probability
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
		if (g_pc < 0) g_pc = 0.8;
		if (g_pm < 0) g_pm = 0.2;

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
				csv << CSV_HEADER << '\n';
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
							    << g_pop << ','
							    << g_pc << ','
							    << g_pm << ','
							    << g_time_limit_sec << ','
							    << fit << ','
								<< g_kp_instance << ','
								<< gens << ','
								<< evals << ','
								<< (g_seed ? to_string(g_seed) : string("")) << ','
								<< selectionToString(g_selection_method) << ','
								<< crossoverToString(g_crossover_method) << ','
								<< mutationToString(g_mutation_method) << ','
								<< g_elite_count << ','
								<< g_inversion_probability << ','
								<< g_uniform_swap_probability
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
