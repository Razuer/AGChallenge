#pragma once

#include "Evaluator.h"
#include <string>
#include <vector>

class CKnapSackEvaluator : public CEvaluator {
public:
    // Configure from instance file and optional optimum file
    // instance format: first line: n capacity; then n lines: value weight
    bool bConfigure(const std::string &sInstancePath, const std::string &sOptimumPath = "");

    double dEvaluate(const int *piGenotype) override;

private:
    std::vector<int> v_values;
    std::vector<int> v_weights;
    int i_capacity = 0;

    double d_sum_values = 0.0; // for normalization when optimum not provided
    double d_optimum = -1.0;   // if provided, used for normalization
};
