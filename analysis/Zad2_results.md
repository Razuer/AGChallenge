# Zadanie 2 – badania porównawcze

Eksperymenty wykonano na instancji `large_scale/knapPI_2_200_1000_1` z prawdziwym optimum (`large_scale-optimum/knapPI_2_200_1000_1`). Każda konfiguracja działała w trybie szybkim przez 2 sekundy, z populacją 200 osobników oraz krzyżowaniem `pc = 0.9`. Wyniki zapisano w plikach CSV (`analysis/zad2_config*.csv`) – każdy plik zawiera pięć niezależnych powtórzeń.

| ID | Selekcja | Krzyżowanie | Mutacja (`pm`) | Elityzm | `p_inv` | Śr. fitness | Najlepsza wartość | Śr. generacje | Śr. ewaluacje | Uwagi |
|----|----------|-------------|----------------|---------|---------|--------------|-------------------|---------------|---------------|-------|
| A  | tournament | one-point | bit-flip (0.01) | 0 | 0.00 | 0.9095 | 0.9291 | 304.0 | 60 183 | Bazowa wersja z zadania 1. |
| B  | tournament | two-point | bit-flip (0.01) | 4 | 0.00 | **0.9983** | **1.0000** | 312.4 | 60 618 | Szybko osiąga optimum dzięki elitaryzmowi. |
| C  | roulette | uniform | swap (0.10) | 4 | 0.00 | 0.7840 | 0.8383 | 460.6 | 82 386 | Wyższa różnorodność kosztem jakości i liczby ewaluacji. |
| D  | random-two | two-point | scramble (0.20) | 2 | 0.05 | 0.9983 | 1.0000 | 4 195.6 | 767 611 | Inwersja + scramble dają wyniki porównywalne z B, ale znacznie droższe obliczeniowo. |

Konfiguracja **B** zapewnia najlepszy kompromis między jakością i kosztem obliczeń – osiąga optimum w większości uruchomień przy liczbie ewaluacji zbliżonej do bazowej wersji. Konfiguracja **D** również dociera do optimum, lecz wymaga ~12× więcej ewaluacji. Konfiguracja **C** zwiększa eksplorację, ale przy tak krótkim budżecie czasowym nie zbliża się do optimum.

Polecenia testowe (przykład dla konfiguracji B):

```bash
./build_zad2/bin/AGChallenge --quick --problem knap \
  --kp-instance instances_01_KP/large_scale/knapPI_2_200_1000_1 \
  --kp-opt instances_01_KP/large_scale-optimum/knapPI_2_200_1000_1 \
  --quick-seconds 2 --runs 5 --pop 200 --pc 0.9 --pm 0.01 \
  --selection tournament --crossover-method two-point \
  --mutation-method bit-flip --elitism 4 --pinv 0 \
  --csv analysis/zad2_configB.csv
```

Analogiczne komendy (z innymi operatorami) znajdują się w historii plików CSV.

Do zbiorczego podsumowania dowolnego pliku wynikowego możesz użyć skryptu `analysis/summarize_batch.py`, np.:

```bash
python analysis/summarize_batch.py analysis/zad2_configB.csv
```
