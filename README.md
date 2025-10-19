# AGChallenge

Genetic Algorithm written in C++.

## Build (VS Code)

-   Make sure you have clang, CMake and Ninja installed
-   Open this folder in VS Code
-   Use CMake Tools: select preset "Ninja + Clang (Release)" or run configure
-   Build the target `AGChallenge`

Alternatively from terminal:

```bash
cmake --preset ninja-clang
cmake --build --preset build -j
```

The binary will be at `build/bin/AGChallenge`. Problem files are copied next to it under `build/bin/problem_files/` and the code also has a compile definition `P3_PROBLEM_FOLDER` pointing to the repository `problem_files` folder.

## Usage

The program can run several benchmark problems and a 0/1 knapsack evaluator. For quick experimentation, use quick mode which runs a single problem for a short, configurable time.

General flags (defaults in parentheses):

-   `--quick` — enable quick mode (skips the long default suite) (default: off)
-   `--quick-seconds <s>` — time budget per run in seconds (default: 10 in quick mode; otherwise 20 min)
-   `--pop <n>` — population size override (default: 100 in quick mode; otherwise 2000)
-   `--pc <p>` — crossover probability override (default: 0.8)
-   `--pm <p>` — mutation probability override (default: 0.2)
-   `--runs <k>` — repeat the quick run k times (default: 1)
-   `--problem <nk|knap|onemax>` — choose problem for quick mode (default: nk)
-   `--kp-instance <path>` — knapsack instance file path when `--problem knap` (default: instances_01_KP/low-dimensional/f1_l-d_kp_10_269)
-   `--kp-opt <path>` — optional knapsack optimum value file (normalizes fitness) (default: none)
-   `--csv <file>` — append results to CSV file (header written if file is new/empty) (default: off)
-   `--batch <file>` — run batch mode: the file specifies instance/optimum paths and a list of configurations (see below)
-   `--seed <u32>` — set RNG seed for reproducible runs (default: random)
-   `--evals <N>` — evaluation budget; stop once this many fitness evaluations are performed (default: disabled)

### Examples

-   NK model (100 bits, k=4), 1 second, 1 run:

    ./build/bin/AGChallenge --quick --problem nk --quick-seconds 1

-   OneMax, 5 seconds, 3 runs, custom GA params:

    ./build/bin/AGChallenge --quick --problem onemax --quick-seconds 5 --runs 3 --pop 100 --pc 0.7 --pm 0.1

-   Knapsack (defaults to f1 low-dimensional instance), 10 seconds, save CSV:

    ./build/bin/AGChallenge --quick --problem knap --quick-seconds 10 --runs 5 --csv results.csv

-   Knapsack with explicit instance and optimum files:

    ./build/bin/AGChallenge --quick --problem knap \
    --kp-instance instances_01_KP/low-dimensional/f1_l-d_kp_10_269 \
    --kp-opt instances_01_KP/low-dimensional-optimum/f1_l-d_kp_10_269 \
    --quick-seconds 10 --runs 5 --csv results.csv

    ./build/bin/AGChallenge --quick --problem knap \
    --kp-instance instances_01_KP/large_scale/knapPI_2_200_1000_1 \
    --kp-opt instances_01_KP/large_scale-optimum/knapPI_2_200_1000_1 \
    --quick-seconds 10 --runs 5 --csv results.csv

CSV columns: `run,problem,pop,pc,pm,seconds,fitness,instance,generations,evaluations,seed`.

Timing note: in quick mode, the timer starts after GA initialization to make short-run comparisons fair (initialization time is not counted). If `--evals` is set, the run stops when either time is up or the evaluation budget is reached — whichever comes first.

## Parameter tuning guide

Below are practical defaults and heuristics that work well with the current GA (tournament selection, one‑point crossover, bit‑flip mutation) and the implemented evaluators.

General heuristics (binary GA):

-   Mutation probability pm (per-bit): start around 1/n, where n is number of bits.
    -   Example: n=200 → pm≈0.005; n=100 → pm≈0.01.
-   Crossover probability pc: 0.8–0.95. Higher pc usually helps exploration.
-   Population size pop: choose a size that allows enough generations within your time budget.
    -   For short runs (≤10–20 s) on n≈100–200, try 100–500.
    -   Very large populations (e.g., 2000) can reduce the number of generations in fixed time and show little benefit unless the run is long.
-   Time budget: compare medians over multiple runs (e.g., --runs 5). Keep seed fixed if you want strict comparability.

Per-problem suggestions:

-   OneMax (easy, separable):
    -   pm≈1/n, pc≈0.8–0.9, pop≈100–200. Should reach fitness=1 quickly.
-   NK (n=100, k=4):
    -   pm≈1/n, pc≈0.9, pop≈200–500. If progress stalls, try slightly higher pm (1.5/n) or longer time.
-   Knapsack 0/1 (this repo):
    -   pm≈1/n is key; too high pm (e.g., 0.2 per bit) destroys building blocks and stalls near ~0.65.
    -   pc≈0.9, pop≈200–500 for large_scale instances (n=200). With these settings we commonly reach near‑optimal or optimal fitness within ~20 s.
    -   If you still plateau, try:
        -   Slightly lower pm (0.5/n) or slightly higher (1.5/n), keep pc high.
        -   Increase time budget rather than population if wall‑time is the constraint.
        -   Optional enhancements (not required): elityzm (1–2 osobniki) lub prosta naprawa rozwiązań przekraczających pojemność.

Examples:

-   Knapsack, large_scale (n=200), tuned params for 20 s:

    ./build/bin/AGChallenge --quick --problem knap \
     --kp-instance instances_01_KP/large_scale/knapPI_2_200_1000_1 \
     --kp-opt instances_01_KP/large_scale-optimum/knapPI_2_200_1000_1 \
     --quick-seconds 20 --runs 5 --pop 200 --pc 0.9 --pm 0.005 --csv results.csv

## Batch mode

To easily compare different parameter sets and save all results to a single CSV file, use the `--batch <file>` flag. The batch format supports two variants:

Common instance/optimum at the top of the file:

```
# comments starting with '#'
instance=instances_01_KP/large_scale/knapPI_2_200_1000_1
optimum=instances_01_KP/large_scale-optimum/knapPI_2_200_1000_1

pop,pc,pm,seconds,runs,seed,evals
200,0.9,0.005,20,5,12345,
200,0.9,0.010,20,5,12345,
300,0.9,0.005,20,3,,
```

Instance/optimum per row (header must start with `instance,optimum,...`):

```
instance,optimum,pop,pc,pm,seconds,runs,seed,evals
instances_01_KP/low-dimensional/f1_l-d_kp_10_269,instances_01_KP/low-dimensional-optimum/f1_l-d_kp_10_269,100,0.9,0.01,10,5,,
instances_01_KP/large_scale/knapPI_2_200_1000_1,instances_01_KP/large_scale-optimum/knapPI_2_200_1000_1,200,0.9,0.005,20,5,42,
```

Notes:
- Required columns: `pop,pc,pm,seconds`. Optional: `runs` (default: 1), `seed` (default: random), `evals` (evaluation budget; default: disabled).
- If you do not pass `--csv`, results will be written to `<your_batch_file>.csv` (a header will be added if the file is new/empty).
- Each configuration is executed `runs` times; each output row contains: `run,problem,pop,pc,pm,seconds,fitness,instance,generations,evaluations,seed`.

Example batch run:

```
./build/bin/AGChallenge --batch examples/knap_batch.csv
```
