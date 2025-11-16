#!/usr/bin/env python3
"""
Quick summarizer for GA batch results, with optional plots.

Usage (summary to stdout):
    python analysis/summarize_batch.py examples/knap_batch_zad2_results.csv

Usage (summary to file + plots):
    python analysis/summarize_batch.py examples/knap_batch_zad2_results.csv \
        --output examples/knap_batch_zad2_summary.csv --plots

The script aggregates runs grouped by the chosen columns and reports:
- count of runs
- average fitness
- best fitness
- population standard deviation of fitness
- average generations
- average evaluations

When `--plots` is enabled and matplotlib/seaborn are available, basic bar plots
of average fitness by `selection`, `crossover` and `mutation` are saved as PNGs
next to the input CSV.
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
    parser.add_argument(
        "--plots",
        action="store_true",
        help="Generate simple PNG plots (avg_fitness vs selection/crossover/mutation).",
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

    if args.plots and summary_rows:
        try:
            import pandas as pd
            import seaborn as sns
            import matplotlib.pyplot as plt
        except ImportError:
            print("Plots requested but pandas/seaborn/matplotlib are not available.", file=sys.stderr)
            return

        df = pd.DataFrame(summary_rows)
        # numeric helpers
        df["avg_fitness_float"] = pd.to_numeric(df.get("avg_fitness", None), errors="coerce")
        df["avg_evaluations_float"] = pd.to_numeric(df.get("avg_evaluations", None), errors="coerce")
        df["pm_float"] = pd.to_numeric(df.get("pm", None), errors="coerce")

        base = args.csv_path.rsplit(".", 1)[0]

        # 1) Barplots: average fitness by single categorical dimension
        for col in ("selection", "crossover", "mutation"):
            if col in df.columns and df[col].nunique(dropna=True) > 1:
                sub = df.dropna(subset=["avg_fitness_float"])
                if sub.empty:
                    continue
                plt.figure(figsize=(6, 4))
                order = sorted(sub[col].dropna().unique())
                sns.barplot(data=sub, x=col, y="avg_fitness_float", order=order, estimator="mean", ci=None)
                plt.ylabel("Average fitness (per configuration)")
                plt.xlabel(col.capitalize())
                plt.tight_layout()
                out_path = f"{base}_by_{col}.png"
                plt.savefig(out_path)
                plt.close()

        # 2) Heatmap: selection x crossover for each pm (multi-dimensional),
        # filtered to a canonical GA setting to make cells comparable.
        # Here: mutation=bit-flip, elitism=4, pinv=0.
        if {"selection", "crossover", "pm_float", "mutation", "elitism", "pinv"}.issubset(df.columns):
            sub = df[
                (df["mutation"] == "bit-flip")
                & (df["elitism"] == "4")
                & (df["pinv"] == "0")
            ].dropna(subset=["avg_fitness_float", "pm_float"])
            if not sub.empty:
                for pm_value, df_pm in sub.groupby("pm_float"):
                    pivot = df_pm.pivot_table(
                        index="selection",
                        columns="crossover",
                        values="avg_fitness_float",
                        aggfunc="mean",
                    )
                    if pivot.empty:
                        continue
                    plt.figure(figsize=(6, 4))
                    sns.heatmap(pivot, annot=True, fmt=".3f", cmap="viridis")
                    plt.title(f"Avg fitness by selection × crossover (pm={pm_value})")
                    plt.ylabel("Selection")
                    plt.xlabel("Crossover")
                    plt.tight_layout()
                    out_path = f"{base}_heatmap_selection_crossover_pm{pm_value}.png"
                    plt.savefig(out_path)
                    plt.close()

        # 3) Heatmap: mutation x pm (averaged over other choices)
        if {"mutation", "pm_float"}.issubset(df.columns):
            sub = df.dropna(subset=["avg_fitness_float", "pm_float"])
            if not sub.empty and sub["mutation"].nunique(dropna=True) > 1:
                pivot = sub.pivot_table(
                    index="mutation",
                    columns="pm_float",
                    values="avg_fitness_float",
                    aggfunc="mean",
                )
                if not pivot.empty:
                    plt.figure(figsize=(6, 4))
                    sns.heatmap(pivot, annot=True, fmt=".3f", cmap="magma")
                    plt.title("Avg fitness by mutation × pm")
                    plt.ylabel("Mutation")
                    plt.xlabel("pm")
                    plt.tight_layout()
                    out_path = f"{base}_heatmap_mutation_pm.png"
                    plt.savefig(out_path)
                    plt.close()

        # 4) Scatter: avg_evaluations vs avg_fitness, colored by mutation, styled by selection
        if "avg_evaluations_float" in df.columns:
            sub = df.dropna(subset=["avg_fitness_float", "avg_evaluations_float"])
            if not sub.empty:
                plt.figure(figsize=(6, 4))
                sns.scatterplot(
                    data=sub,
                    x="avg_evaluations_float",
                    y="avg_fitness_float",
                    hue="mutation" if "mutation" in sub.columns else None,
                    style="selection" if "selection" in sub.columns else None,
                    s=80,
                )
                plt.xlabel("Average evaluations")
                plt.ylabel("Average fitness")
                plt.tight_layout()
                out_path = f"{base}_scatter_evals_vs_fitness.png"
                plt.savefig(out_path)
                plt.close()

        # 5) Elitism effect: barplot for pm=0.01, mutation=bit-flip
        if {"selection", "elitism", "pm_float", "mutation"}.issubset(df.columns):
            sub = df[(df["mutation"] == "bit-flip") & (df["pm_float"] == 0.01)]
            sub = sub.dropna(subset=["avg_fitness_float"])
            if not sub.empty and sub["elitism"].nunique(dropna=True) > 1:
                plt.figure(figsize=(6, 4))
                sns.barplot(
                    data=sub,
                    x="selection",
                    y="avg_fitness_float",
                    hue="elitism",
                    estimator="mean",
                    ci=None,
                )
                plt.ylabel("Average fitness (pm=0.01, bit-flip)")
                plt.xlabel("Selection")
                plt.tight_layout()
                out_path = f"{base}_elitism_by_selection_pm0.01_bitflip.png"
                plt.savefig(out_path)
                plt.close()

        # 6) Inversion effect: heatmap for mutation=scramble (pinv × pm)
        if {"mutation", "pinv", "pm_float"}.issubset(df.columns):
            sub = df[df["mutation"] == "scramble"]
            sub = sub.dropna(subset=["avg_fitness_float", "pm_float"])
            if not sub.empty and sub["pinv"].nunique(dropna=True) > 1:
                pivot = sub.pivot_table(
                    index="pinv",
                    columns="pm_float",
                    values="avg_fitness_float",
                    aggfunc="mean",
                )
                if not pivot.empty:
                    plt.figure(figsize=(6, 4))
                    sns.heatmap(pivot, annot=True, fmt=".3f", cmap="coolwarm")
                    plt.title("Avg fitness for scramble by pinv × pm")
                    plt.ylabel("pinv")
                    plt.xlabel("pm")
                    plt.tight_layout()
                    out_path = f"{base}_heatmap_scramble_pinv_pm.png"
                    plt.savefig(out_path)
                    plt.close()


if __name__ == "__main__":
    main()
