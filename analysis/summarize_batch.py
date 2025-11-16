#!/usr/bin/env python3
"""
Quick summarizer for GA batch results.

Usage:
    python analysis/summarize_batch.py path/to/results.csv \
        [--group-cols selection,crossover,mutation,elitism,pinv,uniform_swap,pop,pc,pm,seconds]

The script prints a CSV summary grouped by the chosen columns, reporting:
- count of runs
- average fitness
- best fitness
- population standard deviation of fitness
- average generations
- average evaluations
"""

import argparse
import csv
import math
import sys
from collections import defaultdict
from statistics import mean, pstdev


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Aggregate GA batch results.")
    parser.add_argument("csv_path", help="Path to the results CSV (produced by --csv or --batch).")
    parser.add_argument(
        "--group-cols",
        default="selection,crossover,mutation,elitism,pinv,uniform_swap,pop,pc,pm,seconds",
        help="Comma-separated list of columns used to group runs (default: %(default)s).",
    )
    parser.add_argument(
        "--output",
        help="Optional output CSV file. When omitted, the summary is printed to stdout.",
    )
    return parser.parse_args()


def try_float(value: str):
    if value is None or value == "":
        return None
    try:
        return float(value)
    except ValueError:
        return None


def aggregate(rows, group_cols):
    buckets = defaultdict(lambda: {"rows": [], "fitness": [], "generations": [], "evaluations": []})

    for row in rows:
        key = tuple(row.get(col, "") for col in group_cols)
        bucket = buckets[key]
        bucket["rows"].append(row)

        fit = try_float(row.get("fitness"))
        gens = try_float(row.get("generations"))
        evals = try_float(row.get("evaluations"))
        if fit is not None and math.isfinite(fit):
            bucket["fitness"].append(fit)
        if gens is not None and math.isfinite(gens):
            bucket["generations"].append(gens)
        if evals is not None and math.isfinite(evals):
            bucket["evaluations"].append(evals)

    summary_rows = []
    for key, data in buckets.items():
        fitness = data["fitness"]
        generations = data["generations"]
        evaluations = data["evaluations"]

        row_out = {col: value for col, value in zip(group_cols, key)}
        row_out["count"] = len(data["rows"])
        row_out["avg_fitness"] = f"{mean(fitness):.6f}" if fitness else ""
        row_out["best_fitness"] = f"{max(fitness):.6f}" if fitness else ""
        row_out["std_fitness"] = f"{pstdev(fitness):.6f}" if len(fitness) > 1 else ""
        row_out["avg_generations"] = f"{mean(generations):.2f}" if generations else ""
        row_out["avg_evaluations"] = f"{mean(evaluations):.2f}" if evaluations else ""
        summary_rows.append(row_out)

    summary_rows.sort(key=lambda r: tuple(r.get(col, "") for col in group_cols))
    return summary_rows


def main():
    args = parse_args()
    group_cols = [c.strip() for c in args.group_cols.split(",") if c.strip()]

    with open(args.csv_path, newline="") as f:
        reader = csv.DictReader(f)
        rows = list(reader)

    summary_rows = aggregate(rows, group_cols)
    fieldnames = group_cols + [
        "count",
        "avg_fitness",
        "best_fitness",
        "std_fitness",
        "avg_generations",
        "avg_evaluations",
    ]

    target = open(args.output, "w", newline="") if args.output else sys.stdout
    writer = csv.DictWriter(target, fieldnames=fieldnames)
    writer.writeheader()
    writer.writerows(summary_rows)
    if args.output:
        target.close()


if __name__ == "__main__":
    main()
