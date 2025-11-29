# Zadanie 2 – badania porównawcze

Eksperymenty wykonano na instancji `large_scale/knapPI_2_200_1000_1` z prawdziwym optimum (`large_scale-optimum/knapPI_2_200_1000_1`). Każda konfiguracja działała w trybie szybkim przez 2 sekundy, z populacją 200 osobników oraz krzyżowaniem `pc = 0.9`. Wyniki zapisano w plikach CSV (`analysis/zad2_config*.csv`) – każdy plik zawiera pięć niezależnych powtórzeń.

| ID  | Selekcja   | Krzyżowanie | Mutacja (`pm`)  | Elityzm | `p_inv` | Śr. fitness | Najlepsza wartość | Śr. generacje | Śr. ewaluacje | Uwagi                                                                                |
| --- | ---------- | ----------- | --------------- | ------- | ------- | ----------- | ----------------- | ------------- | ------------- | ------------------------------------------------------------------------------------ |
| A   | tournament | one-point   | bit-flip (0.01) | 0       | 0.00    | 0.9095      | 0.9291            | 304.0         | 60 183        | Bazowa wersja z zadania 1.                                                           |
| B   | tournament | two-point   | bit-flip (0.01) | 4       | 0.00    | **0.9983**  | **1.0000**        | 312.4         | 60 618        | Szybko osiąga optimum dzięki elitaryzmowi.                                           |
| C   | roulette   | uniform     | swap (0.10)     | 4       | 0.00    | 0.7840      | 0.8383            | 460.6         | 82 386        | Wyższa różnorodność kosztem jakości i liczby ewaluacji.                              |
| D   | random-two | two-point   | scramble (0.20) | 2       | 0.05    | 0.9983      | 1.0000            | 4 195.6       | 767 611       | Inwersja + scramble dają wyniki porównywalne z B, ale znacznie droższe obliczeniowo. |

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

Do zbiorczego podsumowania używamy `analysis/summarize_batch.py`, np.:

```bash
python analysis/summarize_batch.py analysis/zad2_configB.csv
```

## Krótkie podsumowanie zadania 2

**Nowe elementy algorytmu**

-   Selekcje:
    -   _Tournament_ – losujemy 4 osobniki, wybieramy najlepszego; daje silną presję selekcji.
    -   _Random-two_ – losujemy 2 osobniki, wybieramy lepszego; słabsza, ale tańsza selekcja.
    -   _Roulette_ – prawdopodobieństwo wyboru jest proporcjonalne do fitness (z przesunięciem, by wagi były nieujemne).
    -   _Elityzm_ – w każdej generacji kopiujemy bez zmian `k` najlepszych osobników (w eksperymentach najczęściej `k = 4`), niezależnie od typu selekcji.
-   Krzyżowania:
    -   _One-point_ – klasyczne, wymiana ogona chromosomu za jednym punktem.
    -   _Two-point_ – wymiana fragmentu między dwoma punktami.
    -   _Uniform_ – dla każdego genu z prawdopodobieństwem ok. 0.5 zamieniamy bity rodziców (parametr `uniform_swap`).
-   Mutacje:
    -   _Bit-flip_ – każdy bit odwracany z prawdopodobieństwem `pm` (np. 0.01).
    -   _Swap_ – zamiana dwóch losowych pozycji w chromosomie.
    -   _Scramble_ – losowy fragment chromosomu jest tasowany (permutacja).
-   Inwersja:
    -   Losowane są dwa indeksy, a fragment między nimi jest odwracany; operator stosowany z prawdopodobieństwem `pinv` (np. 0.05) po mutacji, zwiększa eksplorację przy scramble.

**Skrócona analiza wyników (knapPI_2_200_1000_1)**

-   Najlepsza konfiguracja jakość/koszt: selekcja _tournament_ + crossover _uniform_ lub _two-point_ + mutacja _bit-flip_, `pm ≈ 0.01`, elityzm `4`, `pinv = 0` – średni fitness ≈ 1.0, praktycznie zawsze osiągane optimum przy umiarkowanej liczbie ewaluacji (~6·10⁴).
-   Elityzm wyraźnie podnosi fitness (np. dla tournament two-point `pm = 0.01` przejście z elityzmu 0→4 podnosi średni fitness z ok. 0.94 do ≈1.0).
-   Typ selekcji: _tournament_ jest najstabilniejszy i daje najwyższe wyniki; _random-two_ jest poprawny, ale słabszy; _roulette_ zwykle kończy w okolicy 0.65–0.7, nawet z elityzmem.
-   Typ mutacji i `pm`: _bit-flip_ przy małym `pm` (ok. 0.01) jest najlepszy; zwiększanie `pm` do 0.1–0.2 obniża fitness. _Swap_ i _scramble_ mogą dojść do optimum, ale wymagają zdecydowanie większej liczby ewaluacji (rzędu 10⁶).
-   Inwersja (`pinv`) dla scramble poprawia eksplorację i może podnieść wynik, ale jeszcze bardziej zwiększa koszt obliczeniowy, więc nie jest najlepsza przy ograniczonym budżecie czasowym.
