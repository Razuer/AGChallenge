#include "Knapsack.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

using namespace std;

bool CKnapSackEvaluator::bConfigure(const std::string &sInstancePath, const std::string &sOptimumPath)
{
    ifstream in(sInstancePath);
    if (!in) return false;

    int n;
    in >> n >> i_capacity;
    if (!in || n <= 0 || i_capacity <= 0) return false;

    v_values.resize(n);
    v_weights.resize(n);
    d_sum_values = 0.0;
    for (int i = 0; i < n; ++i)
    {
        int val, wt;
        in >> val >> wt;
        if (!in) return false;
        v_values[i] = val;
        v_weights[i] = wt;
        d_sum_values += max(0, val);
    }
    in.close();

    // set evaluator base params
    CEvaluator::bConfigure(n, 1.0);

    // optional optimum
    d_optimum = -1.0;
    if (!sOptimumPath.empty())
    {
        ifstream opt(sOptimumPath);
        if (opt)
        {
            double optVal = -1.0;
            opt >> optVal;
            if (opt && optVal > 0) d_optimum = optVal;
        }
    }
    return true;
}

double CKnapSackEvaluator::dEvaluate(const int *piGenotype)
{
    if (!piGenotype || (int)v_values.size() != i_number_of_bits) return dWRONG_VALUE;

    long long totalValue = 0;
    long long totalWeight = 0;

    for (int i = 0; i < i_number_of_bits; ++i)
    {
        int bit = piGenotype[i] ? 1 : 0;
        if (bit)
        {
            totalValue += v_values[i];
            totalWeight += v_weights[i];
        }
    }

    // If overweight, apply a smooth penalty proportional to capacity/weight
    if (totalWeight > i_capacity)
    {
        double ratio = (double)i_capacity / (double)totalWeight; // in (0,1)
        double base = (d_optimum > 0 ? totalValue / d_optimum : totalValue / max(1.0, d_sum_values));
        return min(1.0, max(0.0, base * ratio));
    }
    else
    {
        if (d_optimum > 0)
            return min(1.0, (double)totalValue / d_optimum);
        else
            return min(1.0, (double)totalValue / max(1.0, d_sum_values));
    }
}
