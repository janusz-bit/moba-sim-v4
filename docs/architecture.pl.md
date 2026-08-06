# Architektura

Bezgłowe (headless) jądro symulacji MOBA z opcjonalną warstwą widoku opartą na SDL3.
Ten dokument opisuje warstwy, niezmienniki, które je spajają, oraz — tam, gdzie
decyzja nie jest oczywista — stojące za nią uzasadnienie.

Komendy budowania i codzienny przepływ pracy opisano w [`AGENTS.md`](../AGENTS.md).

## Spis treści

- [Warstwy](#warstwy) · [Zasady projektowe](#zasady-projektowe)
- [`stats/` — potok statystyk](#stats--potok-statystyk)
- [`sim/` — czas symulacji](#sim--czas-symulacji)
- [`effects/` — wzmocnienia, osłabienia, aury](#effects--wzmocnienia-osłabienia-aury)
- [`champions/` — wyznaczanie stanu jednostki](#champions--wyznaczanie-stanu-jednostki)
- [`items/`](#items) · [`events/`](#events) · [`view/`](#view)
- [Przepływ danych jednego kroku symulacji](#przepływ-danych-jednego-kroku-symulacji)
- [Niezmienniki](#niezmienniki) · [Rozszerzanie systemu](#rozszerzanie-systemu)
- [Strategia testowania](#strategia-testowania) · [Jeszcze niezaimplementowane](#jeszcze-niezaimplementowane)

## Warstwy

```
                    ┌─────────────────────────────────────────┐
                    │  moba_sim_view  (SDL3)                  │
                    │  window · renderer2d · game_loop        │
                    └───────────────────┬─────────────────────┘
                                        │ odczyty Tick / statystyk
╔═══════════════════════════════════════▼═════════════════════════════════════╗
║  moba_sim_core  (bez SDL, bez I/O, headless)                                ║
║                                                                             ║
║   champions/     Champion: posiada źródła, wyznacza z nich StatTable        ║
║       │                                                                     ║
║       ├── items/        Item: nazwany worek Modyfikatorów                   ║
║       │                                                                     ║
║       ├── effects/      Effect · EffectSet · Lifetime · StackPolicy         ║
║       │        │        wzmocnienia, osłabienia, aury, pasywki przedmiotów  ║
║       │        │                                                            ║
║       │        └── sim/     Tick · TickSpan · TickRate (czas całkowity)     ║
║       │                                                                     ║
║       └── stats/        StatId · Modifier · StatPipeline · StatTable        ║
║                         StatMask · StatBreakdown                            ║
║                                                                             ║
║   events/        wariant Event, rekurencyjna dyspozycja (dziś niezależny)   ║
╚═════════════════════════════════════════════════════════════════════════════╝
```

Zależności wskazują wyłącznie w dół. `stats/` nie wie nic o efektach,
mistrzach ani czasie; `effects/` zna `stats/` i `sim/`, ale nie mistrzów;
`champions/` komponuje wszystko, co znajduje się poniżej.

`moba_sim_core` nigdy nie może linkować SDL ani wykonywać operacji I/O.
Symulacja musi dać się uruchamiać bezgłowo, w teście, albo tysiące razy z rzędu
na potrzeby analizy balansu, a CI uruchamia ją bez wyświetlacza.

## Zasady projektowe

Cztery idee wyjaśniają większą część kodu.

**1. Zachowanie jest danymi wszędzie tam, gdzie framework musi o nim rozumować.**
Tożsamość, zależności, czas życia i reguła stackowania efektu to zwykłe
struktury, a nie logika ukryta w funkcji wywoływalnej. Funkcją jest wyłącznie
arytmetyka. Framework, który nie widzi, *od czego* zależy efekt, nie potrafi
efektów uporządkować, więc jest skazany na iterowanie, aż liczby przestaną się
zmieniać; ten, który nie widzi, *kiedy* efekt się kończy, nie potrafi go
odświeżyć, zdjąć ani pokazać timera wzmocnienia. Zadeklarowanie tego jako danych
zwraca się natychmiast — patrz
[Effects](#effects--wzmocnienia-osłabienia-aury).

**2. Dokładne zamiast przybliżonego.** Czas jest całkowitoliczbowy, nie jest
akumulacją sekund w `double`. Wyznaczanie statystyk to jeden przejście w
porządku topologicznym, a nie iteracja do osiągnięcia tolerancji. Ani wyniki,
ani wygasanie nie mogą zależeć od tempa klatek ani od ustawień solvera —
inaczej powtórka (replay) nie jest odtwarzalna, a testy wszędzie potrzebują
epsilonów.

**3. Każda liczba jest możliwa do wyśledzenia.** Każdy `Modifier` niesie etykietę
`source`, a `format_breakdown` renderuje pełne wyprowadzenie dowolnej
statystyki. Na pytanie „dlaczego moje AD wynosi 217.8” musi zawsze dać się
odpowiedzieć — to właśnie jest głównym *celem* symulatora.

**4. Stan pochodny jest pochodny.** `StatTable` mistrza jest czystą funkcją
danych bazowych, poziomu, przedmiotów i efektów. Jest cache'owany, ale cache
jest optymalizacją, nigdy źródłem prawdy: wyrzucenie go i przebudowanie musi
dać tę samą odpowiedź.

## `stats/` — potok statystyk

### Formuła

Każda statystyka jest wyznaczana przez trzy wiadra, wzorowane na potoku obrażeń
z Path of Exile:

```
value = sum(Base) * (1 + sum(Inc)) * product(1 + More)
```

| Wiadro | Sposób łączenia | Przykład |
|---|---|---|
| `Base` | addytywnie | `+40 AD` z B.F. Sword |
| `Inc`  | addytywnie w jeden mnożnik | `+10%` i `+20%` → `× 1.3` |
| `More` | multiplikatywnie, każde osobno | `+10%` i `+20%` → `× 1.1 × 1.2` |

`Inc` i `More` różnią się tym, jak stackują się *wzajemnie* ze sobą — i to jest
cały powód istnienia obu: procenty addytywne rozmywają się w miarę dodawania
kolejnych, multiplikatywne nie.

Pusty potok daje w wyniku `0`, a `Inc`/`More` bez żadnego `Base` również daje
`0` — procent z niczego to nic.

### Typy

| Typ | Plik | Rola |
|---|---|---|
| `StatId` | `stat_id.hpp` | która statystyka, plus wartownik `Count` |
| `Modifier` | `modifier.hpp` | jedna oznaczona zmiana: statystyka, rodzaj, wartość, źródło |
| `ModifierKind` | `modifier.hpp` | `Base` / `Inc` / `More` |
| `StatPipeline` | `stat_pipeline.hpp` | trzy wiadra dla jednej statystyki |
| `StatTable` | `stat_table.hpp` | jeden potok na statystykę: stan statystyk jednostki |
| `StatMask` | `stat_mask.hpp` | zbiór statystyk (`std::bitset<kStatCount>`) |
| `StatBreakdown` | `stat_breakdown.hpp` | pochodzenie jednej wyliczonej wartości |

### `StatId` nie może się rozjechać

`StatId` kończy się wartownikiem `Count`, a wszystko pozostałe jest z niego
wyprowadzane:

```cpp
inline constexpr std::size_t kStatCount = static_cast<std::size_t>(StatId::Count);
inline constexpr std::array<StatId, kStatCount> kAllStats = /* generowane */;
```

Każda tablica indeksowana przez `stat_index()` ma rozmiar `kStatCount` i zawiera
`static_assert`, więc zapomnienie wpisu jest błędem kompilacji, a nie
odczytem poza zakresem. Robią tak zarówno `kStatNames` (`stat_id.hpp`), jak i
`kStatSpecs` (`champions/champion.cpp`).

Żaden kod nie robi `switch` po `StatId`: `switch` wygenerowałby jedynie
ostrzeżenie `-Wswitch` dla brakującego przypadku, a potem niezdefiniowane
zachowanie, podczas gdy tablica ze `static_assert` wywala build.

### Jeden typ `Modifier` dla każdego źródła

`Modifier` jest celowo neutralny względem źródła — przedmioty, efekty i
statystyki bazowe mistrza tworzą ten sam typ, a wszystkie trafiają do potoku
przez pojedyncze `apply_modifier` w `modifier.cpp`:

```cpp
void apply_modifier(StatPipeline& pipe, const Modifier& mod, const std::string& source);
```

Ta funkcja to jedyne miejsce mapujące `ModifierKind` na `add_base` /
`add_inc` / `add_more`. Gdy każde źródło miało własną kopię tego mapowania,
mogłyby się różnić w interpretacji rodzajów; teraz nie ma czego interpretować
różnie. Pomocnicze `base_mod(stat, value, source)`, `inc_mod`, `more_mod`
utrzymują czytelność miejsc wywołania.

### Pochodzenie (provenance)

Potoki przechowują `StatBreakdown::Entry{value, source}` zamiast gołych
wartości double, więc `breakdown()` zwraca każdy wkład wraz z jego etykietą,
a `format_breakdown` go renderuje:

```
AttackDamage = 138
  Base = 138
    + 68  (Ahri base, lvl 6)
    + 40  (B.F. Sword)
    + 30  (Ahri (Fury))
  138 * 1 * 1 = 138
```

Dlatego przewlekanie etykiet pojawia się w całym stosie: efekty oznaczają swoje
wyjście swoim `EffectKey`, przedmioty swoją nazwą, statystyki bazowe
`"<name> base, lvl <n>"`. Pochodzenie jest funkcją pierwszej klasy, a nie
rusztowaniem debugowym.

## `sim/` — czas symulacji

`Tick` to **punkt** w czasie, `TickSpan` to **czas trwania**, a `TickRate` to
jedyne miejsce, które wie, ile ticków składa się na sekundę. Rozróżnienie to
jest egzekwowane przez system typów: `Tick - Tick` daje `TickSpan`,
`Tick + TickSpan` daje `Tick`, a `Tick + Tick` się nie kompiluje.

### Dlaczego liczby całkowite

Załóżmy, że czas byłby akumulowanym `double` w sekundach. Wzmocnienie nałożone
w chwili `t` o czasie trwania 3 sekund wygasa, gdy `now - t >= 3.0`. Dodanie
`1.0/60.0` do `double` 180 razy nie daje dokładnie `3.0`, więc porównanie
odpala tick wcześniej lub później, zależnie od tempa ticków i od tego, gdzie
w przebiegu to następuje. Konsekwencje:

- ten sam scenariusz daje różne liczby przy innym tempie ticków;
- powtórka nie jest bitowo odtwarzalna, co czyni desynchronizacje i regresje
  niemożliwymi do wyśledzenia;
- każdy test czasu trwania potrzebuje epsilona, a testy oparte na epsilonach
  ukrywają błędy pomyłki o jeden (off-by-one).

Przy całkowitoliczbowych tickach `now >= expires_at` jest dokładne.
`TickSpan{180}` to 3 sekundy przy 60 tickach/s, zawsze i wszędzie.
`tests/tick.test.cpp` sprawdza, że czas trwania 3 sekund jest dokładny przy
10, 30, 60 i 128 tickach na sekundę.

Sekundy istnieją tylko na granicy: `TickRate::ticks_from_seconds` (z
zaokrąglaniem, żeby krótkie czasy trwania nie kolapsowały do zera) przelicza
dane gry do środka, a `seconds_from_ticks` / `seconds_at` przeliczają z
powrotem na zewnątrz na potrzeby wyświetlania lub efektów, które rzeczywiście
myślą w sekundach. Niedodatnie tempo jest przycinane do 1 zamiast dzielenia
przez zero.

`Tick`, `TickSpan` i `TickRate` nadają się do użycia w `constexpr`, więc czasy
trwania pochodzące z danych statycznych mogą być stałymi czasu kompilacji.

## `effects/` — wzmocnienia, osłabienia, aury

To najważniejsza część projektu, więc warto najpierw przedstawić kształt,
a uzasadnienie potem.

### Kształt

```cpp
struct Effect {
    EffectKey key{};        // stabilna tożsamość — steruje stackowaniem
    StatMask reads{};       // statystyki, które contribute() może odczytać (musi być kompletne)
    StatMask writes{};      // statystyki, które contribute() może zmienić (musi być kompletne)
    Lifetime lifetime = Permanent{};
    StackPolicy policy = StackPolicy::Refresh;
    double magnitude = 0.0; // porównywane przez ReplaceIfStronger
    StackCount max_stacks = 1;
    std::function<void(const EffectContext&, ModifierSink&)> contribute{};
};
```

Wszystko poza `contribute` jest danymi możliwymi do inspekcji. I o to właśnie
chodzi: framework potrafi porządkować, wygaszać, odświeżać, stackować, zdejmować
i raportować efekty właśnie dlatego, że widzi te pola.

W typowym przypadku — płaski bonus bez zależności — lambda nie jest potrzebna:

```cpp
champion.apply_effect(flat_effect({.source = "Baron", .name = "Hand of Baron"},
                                  {base_mod(StatId::AttackDamage, 20)},
                                  Timed::for_span(champion.now(), TickSpan{180})));
```

### Tożsamość: `EffectKey`, a nie licznik

```cpp
struct EffectKey { std::string source; std::string name; };
```

Tożsamość jest *semantyczna*: kto nałożył i który to efekt. Q Ahri zawsze daje
`{"Ahri", "Q"}`, więc ponowne rzucenie jest rozpoznawane jako to samo
wzmocnienie i odświeża je zamiast stackować.

Automatycznie inkrementowany licznik zawiódłby dokładnie w tym miejscu. To samo
wzmocnienie rzucone dwa razy dostaje dwa identyfikatory i stackuje się po
cichu; identyfikatory zależą od tego, ile efektów zostało wcześniej utworzonych
w przebiegu, więc nie są odtwarzalne między przebiegami ani zapisywalne jako
literały w testach. Klucze semantyczne są też porównywalne, wypisywalne i
serializowalne — to warunki konieczne dla powtórek (replay).

Tożsamość jest trzymana oddzielnie od `EffectHandle`, który odnosi się do
*jednej konkretnej instancji* i istnieje tylko po to, by usunąć jedną z wielu
instancji `Independent`. Pomieszanie „który to efekt” z „która to instancja”
jest tym, co czyni reguły stackowania niemożliwymi do wyrażenia.

### Czas życia jako dane

```cpp
using Lifetime = std::variant<Permanent, Timed, OneShot, Until>;
```

| Alternatywa | Znaczenie |
|---|---|
| `Permanent` | żyje do jawnego usunięcia — pasywki przedmiotów, aury w zasięgu |
| `Timed{expires_at}` | wygasa w ustalonym ticku; tworzone przez `Timed::for_span(now, duration)` |
| `OneShot` | wnosi wkład przez dokładnie jeden krok |
| `Until{predicate}` | furtka awaryjna dla „dopóki HP poniżej 50%” |

Kuszącą alternatywą jest pozwolić każdemu efektowi pilnować się samemu:
zwracać flagę `alive` i porównywać `now - start >= duration` wewnętrznie.
Kosztuje to więcej, niż oszczędza:

- **brak odświeżania i przedłużania** — nic poza efektem nie może przesunąć
  terminu, którego nie widzi, więc „twoje wzmocnienia trwają 20% dłużej” jest
  niezaimplementowalne;
- **brak zdejmowania (dispel)** — oczyszczanie wymaga wiedzy, co jest
  czasowym osłabieniem;
- **brak paska wzmocnień** — na `remaining()` nie da się odpowiedzieć, gdy
  termin żyje w przechwyceniu lambdy;
- **brak harmonogramowania** — każdy efekt musi być odpytywany w każdym kroku,
  na zawsze;
- **brak serializacji** — `start_time` w przechwyceniu nie daje się zapisać,
  a skopiowanie właściciela albo go współdzieli, albo duplikuje, zależnie od
  tego, jak napisano przechwycenie;
- **zduplikowana arytmetyka** — to samo porównanie wygasania w każdym efekcie
  czasowym.

`Until` zachowuje furtkę awaryjną dla prawdziwie niestandardowych czasów życia,
ale jako jedną z czterech alternatyw, a nie jedyny mechanizm.

### Polityki stackowania

Ponowne nałożenie efektu, którego klucz już jest aktywny, rozstrzyga
`StackPolicy`, wybierana jawnie w miejscu nałożenia, bo błędny wybór to błąd
balansu:

| Polityka | Zachowanie |
|---|---|
| `Refresh` | resetuje czas trwania, jedna instancja (typowe ponowne rzucenie wzmocnienia) |
| `Stack` | dodaje stack intensywności do limitu `max_stacks`, odświeża czas trwania |
| `ExtendDuration` | dodaje nowy czas trwania do pozostałego, więc wczesne ponowne rzucenia nic nie marnują |
| `IgnoreIfPresent` | odrzuca nałożenie, póki instancja jest aktywna |
| `ReplaceIfStronger` | zastępuje tylko przy wyższej `magnitude`, albo równej z późniejszym wygaśnięciem |
| `Independent` | współistnieją; każda instancja ma własny czas życia |

`Stack` automatycznie skaluje wkłady: `ModifierSink` mnoży `Base` i `Inc`
przez liczbę stacków, podczas gdy `More` **się składa (compounds)** — N stacków
`+10%` staje się `1.1^N - 1`, zgodnie z „efekt nałożony N razy”, a nie sumą
liniową. Zestackowane wkłady są oznaczane w breakdownach jako
`"Nasus (Siphoning Strike) x3"`.

### Dwie fazy, celowo oddzielone

```cpp
std::vector<EffectKey> advance(Tick now);                                   // mutująca
void contribute_all(StatTable&, Tick now, TickRate) const;                  // czysta
```

Mają przeciwstawne wymagania, więc są różnymi funkcjami:

- `advance` uruchamiane jest **dokładnie raz na krok symulacji**. Wygasza
  zakończone efekty, usuwa zużyte `OneShot` i zwraca klucze, które zniknęły,
  żeby wywołujący mógł zareagować.
- `contribute_all` jest **czysta** i może być uruchamiana tak często, jak
  potrzebne są statystyki. Nigdy niczego nie usuwa.

Ich połączenie wymusza sprzeczność. Jeśli funkcja licząca statystyki
jednocześnie honoruje usuwanie, a statystyki muszą być liczone więcej niż raz
(jak wymaga każdy schemat iteracyjny), to albo usuwanie dzieje się w jakimś
dowolnym przebiegu, albo każdy efekt uboczny odpala nieprzewidywalną liczbę
razy. Rozdzielenie czyni `contribute` czystą z konstrukcji:
`tests/effect.test.cpp` wyznacza `OneShot` dwadzieścia razy w obrębie jednego
kroku i nie jest on ani zużywany, ani podwajany.

`contribute` **nie może mieć efektów ubocznych.** Uruchamiana jest przy każdej
przebudowie statystyk. Wszystko, co musi wydarzyć się raz na krok, należy do
`advance`.

### Porządkowanie: jedno dokładne przejście, bez punktu stałego

Ponieważ każdy efekt deklaruje `reads` i `writes`, graf zależności jest znany
zanim wykona się jakakolwiek arytmetyka. `EffectSet` sortuje go topologicznie
i wyznacza każdy efekt raz, więc efekt czytający statystykę widzi jej
**ostateczną** wartość:

```cpp
// 50% całkowitego AD jako pancerz — widzi AD po przedmiotach i wszystkich
// innych efektach AD, niezależnie od kolejności nakładania.
Effect{
    .key = {.source = "Steelcaps", .name = "Sturdy"},
    .reads = {StatId::AttackDamage},
    .writes = {StatId::Armor},
    .contribute = [](const EffectContext& ctx, ModifierSink& sink) {
        sink.add_base(StatId::Armor, 0.5 * ctx.stats(StatId::AttackDamage));
    },
};
```

Alternatywa — iterowanie całego zbioru efektów, aż wartości przestaną się
zmieniać w ramach jakiegoś `eps` — ma realne problemy:

- **wyniki zależą od tolerancji**, więc `eps` staje się parametrem balansu;
- **zbieżność nie jest gwarantowana**, więc legalny build przedmiotów może
  rzucić `ConvergenceError` w środku symulacji, bez rozsądnego sposobu
  wyjścia;
- **w dziedzinie nie ma punktu stałego do znalezienia.** Interakcje statystyk
  MOBA to DAG: Rabadon mnoży całkowite AP, ale wkłady AP nie zasilają samych
  siebie. Iteracja rozwiązuje problem, którego dziedzina nie ma.

Dwa szczegóły czynią to praktycznym:

**Warstwowanie to nie cykl.** Efekt, który czyta statystykę, którą również
zapisuje, jest dozwolony — „zyskaj 10% całkowitego AD jako bonusowe AD” czyta
wartość zgromadzoną do tej pory i dodaje dokładnie jedną warstwę wzmocnienia.
To jest dobrze zdefiniowane i skończone; to nie jest żądanie sumowania szeregu
geometrycznego.

**Rówieśnicy (peers) to nie cykl.** Dwa efekty, które obie czytają *i* zapisują
tę samą statystykę, są rówieśnikami, a nie zależnymi — dwa wzmacniacze AD nie
mogą czekać nawzajem na siebie. `depends_on` w `effect_set.cpp` pomija więc
krawędź, gdy obie strony czytają i zapisują tę samą statystykę. Wyjątek jest
wąski: prawdziwy cykl konwersji (AD → Armor oraz Armor → AD) ma każdy efekt
czytający statystykę, której nie zapisuje, więc obie krawędzie przetrwają i
cykl nadal zostanie wykryty.

Prawdziwe cykle są odrzucane przez `EffectSet::apply` błędem `EffectCycleError`,
wymieniającym zaangażowane efekty, a zbiór pozostaje niezmieniony. Odrzucanie w
momencie nakładania jest właściwe, bo cykl to **błąd modelowania** — wartość,
jaką taka para miałaby wytworzyć, jest niezdefiniowana — a czytelny komunikat
jest lepszy niż numeryczna awaria w gorącej ścieżce.

Sortowanie jest deterministyczne (algorytm Kahna, zawsze wybierający
najniższy gotowy indeks). Porządek nie może zależeć od iteracji po hashu ani od
wartości wskaźników: gdy w grę wchodzą modyfikatory `More`, inny porządek może
dać różne wyniki zmiennoprzecinkowe. Wyliczony porządek jest cache'owany i
unieważniany tylko wtedy, gdy zmienia się zbiór, bo zależy od zbioru efektów,
a nigdy od wartości statystyk.

### Deklaracje są egzekwowane

Gwarancja porządkowania jest poprawna tylko wtedy, gdy `reads`/`writes` są
kompletne, więc są sprawdzane, a nie przyjmowane na wiarę. `StatView` i
`ModifierSink` rzucają `UndeclaredStatAccess` — podając nazwę efektu i
statystyki — przy każdym dostępie poza zadeklarowanymi maskami. Brakująca
deklaracja to głośna awaria zamiast po cichu nieaktualnej liczby.

## `champions/` — wyznaczanie stanu jednostki

`ChampionData` przechowuje statystyki bazowe w formacie wiki: wartość dla
poziomu 1 oraz przyrost na poziom dla każdej z dziesięciu statystyk,
wyznaczane przez `base_value(stat, level) = base + growth * (level - 1)`.
`kStatSpecs` mapuje każdy `StatId` na jego parę wskaźników do członków
`(base, growth)`.

`Champion` posiada **każde** źródło modyfikatorów i wyznacza je w ustalonej
kolejności:

```
StatTable  =  statystyki bazowe na poziomie   →   przedmioty (kolejność założenia)   →   efekty (porządek zależności)
```

### Każdy modyfikator ma właściciela

Celowo **nie ma** możliwości wepchnięcia modyfikatora bezpośrednio do potoku.
`Champion::pipeline(stat)` zwraca `const&`. Jedyne punkty wejścia to `equip`,
`apply_effect` i `set_level`.

To zamyka realną pułapkę. Przebudowa to sposób, w jaki działa usuwanie —
wyciągnięcie pojedynczego modyfikatora z wiadra nie jest obsługiwane, więc
`unequip` przebudowuje z pozostałych źródeł. Modyfikator wepchnięty bezpośrednio
do potoku nie ma właściciela, więc nie jest częścią żadnego źródła, więc
następna przebudowa po cichu go usuwa. W poprzednim projekcie zdjęcie butów
kasowało też wszystkie wzmocnienia. Teraz tymczasowe bonusy to efekty, a stałe
wyposażenie to przedmioty; oba przetrwają, a `tests/champion.test.cpp`
przytwierdza to zachowanie.

### Leniwe, czyste odczyty

Mutacja oznacza tabelę jako brudną; następny odczyt ją przebudowuje:

```cpp
mutable StatTable stats_;
mutable bool dirty_ = true;
```

Odczyt statystyki jest `const` i czysty — dziesiąty odczyt jest równy
pierwszemu. Cache `mutable` to optymalizacja, nie stan. Mutują tylko
`advance_to` / `advance_by`, `apply_effect`, `remove_effect`, `equip`,
`unequip`, `set_level` i `set_tick_rate`.

`advance_to` zawsze unieważnia, nawet gdy nic nie wygasło: sam czas zmienia to,
co efekty wnoszą (efekt skalujący się z upływem czasu, efekt `Timed` w swoim
ostatnim ticku).

### Krokowanie czasu

```cpp
std::vector<EffectKey> advance_to(Tick now);   // bezwzględne
std::vector<EffectKey> advance_by(TickSpan);   // względne
```

Obie zwracają klucze, które wygasły podczas kroku — tak wywołujący loguje
„wzmocnienie się skończyło” albo odpala zdarzenie następcze bez odpytywania.

## `items/`

`Item` to nazwa plus `std::vector<Modifier>` — czyste dane bez zachowania.
Modyfikatory są tego samego typu `Modifier`, który tworzą efekty, więc
`Champion` stosuje oba jedną ścieżką.

`source` modyfikatora przedmiotu jest zwykle puste i wypełniane nazwą
przedmiotu w momencie przebudowy; jawne ustawienie przypisuje linię czemuś
bardziej szczegółowemu, np. `"Zeal (passive)"`. Tożsamość przedmiotu jest po
nazwie, a `unequip` usuwa pierwsze dopasowanie.

## `events/`

System zdarzeń oparty na `std::variant` z rekurencyjnym zagnieżdżaniem:

```cpp
struct EventSequence;                        // zadeklarowany z przodu dla aliasu
using Event = std::variant<PlayerDiedEvent, KeyPressedEvent,
                           std::shared_ptr<EventSequence>>;
struct EventSequence { std::vector<Event> events; };
```

`process_event` dysponuje przez `std::visit` do wolnych przeciążeń
`handle_event`, każde we własnej jednostce translacji, rozstrzyganych w miejscu
wywołania w `event.cpp`. Handler `EventSequence` rekursuje przez
`process_event`, więc sekwencje zagnieżdżają się dowolnie. `debug_out` domyślnie
jest strumieniem badbit, co czyni obsługę zdarzeń cichą, o ile nie poda się
strumienia.

Ta warstwa jest **dziś niezależna**: handlery tylko piszą do strumienia i nie
mają dostępu do `Champion`. Połączenie jej z symulacją to oczywisty następny
krok — patrz [Jeszcze niezaimplementowane](#jeszcze-niezaimplementowane).

## `view/`

Jedyna biblioteka zależna od SDL, utrzymywana celowo jako cienka:

| Typ | Rola |
|---|---|
| `Window` | RAII nad `SDL_Window` + `SDL_Renderer`; niekopiowalne, nieprzenośne; rzuca przy błędzie |
| `Renderer2D` | kształty w trybie immediate, przestrzeń pikseli, y w dół |
| `GameLoop` | pętla o stałym kroku czasowym z akumulatorem |
| `Vec2`, `Rect`, `Color` | geometria i kolory tylko w nagłówkach |

`GameLoop` to pętla [„Fix Your
Timestep”](https://gafferongames.com/post/fix_your_timestep/): `update(dt)`
wykonuje się zero lub więcej razy na klatkę ze stałym `dt`, `render` raz, a
długie przestoje są przycinane do 0,25 s, żeby pauza debugera nie wywołała
serii kroków nadganiających.

Jądro nigdy nie widzi czasu zegara ściennego. `demo.cpp` uruchamia pętlowanie
z tempem ticków zgodnym z tempem mistrza, więc każde `update` to dokładnie
jeden `TickSpan{1}` — zegar nanosekundowy SDL decyduje, *kiedy* zrobić krok,
nigdy *ile* zasymulowanego czasu upłynęło.

## Przepływ danych jednego kroku symulacji

```
pętla gry (lub test) decyduje, że nastąpił krok
        │
        ▼
champion.advance_by(TickSpan{1})
        │
        ├── now_ += 1
        ├── EffectSet::advance(now_)      MUTUJĄCE, dokładnie raz
        │      ├── usuń efekty, których Lifetime dobiegł końca
        │      ├── usuń OneShoty nałożone przed tym tickiem
        │      └── zwróć wygasłe klucze ──────────────► wywołujący reaguje
        └── oznacz statystyki jako brudne
        │
        ▼
champion.compute(StatId::AttackDamage)    (dowolną liczbę razy)
        │
        └── jeśli brudne: rebuild()       CZYSTE
               ├── 1. statystyki bazowe na poziomie   → StatTable
               ├── 2. przedmioty w kolejności założenia → StatTable
               └── 3. EffectSet::contribute_all(table, now_, rate)
                        └── dla każdego efektu w porządku topologicznym:
                               StatView (zadeklarowane reads)  ──┐
                               ModifierSink (zadeklarowane writes) ──► StatTable
        │
        ▼
double, oraz champion.explain(stat) dla pełnego wyprowadzenia
```

## Niezmienniki

Złam je, a rzeczy będą padać cicho zamiast głośno.

**Statystyki**

1. `value = sum(Base) * (1 + sum(Inc)) * product(1 + More)`.
2. Każdy modyfikator trafia do potoku przez `apply_modifier`.
3. Każdy modyfikator niesie `source`; pochodzenie jest funkcją.
4. Żadnego `switch` po `StatId`; używaj tablicy o rozmiarze `kStatCount` ze
   `static_assert`. Nigdy nie przechowuj `StatId::Count`.

**Czas**

5. Czas symulacji jest całkowitoliczbowy. Nigdy nie akumuluj sekund jako
   `double` w jądrze.
6. `TickRate` to jedyna granica sekund.

**Efekty**

7. `contribute` jest czyste. Efekty uboczne należą do `advance`.
8. `reads`/`writes` muszą być kompletne — egzekwowane przez
   `UndeclaredStatAccess`.
9. Wyznaczanie to jedno topologiczne przejście. Bez iteracji, bez epsilona.
10. Cykle są odrzucane przy `apply`, a nie odkrywane podczas wyznaczania.
11. Porządek wyznaczania jest deterministyczny.

**Mistrzowie**

12. Każdy modyfikator ma właściciela: dane bazowe, `items()` lub `effects()`.
13. Odczyt statystyki jest const i czysty; cache nie jest stanem.
14. `StatTable` jest w pełni odtwarzalna z posiadanych źródeł.

**Warstwowanie**

15. `moba_sim_core` nigdy nie linkuje SDL i nigdy nie robi I/O.
16. Zależności wskazują w dół: `stats/` → `sim/` → `effects/` → `champions/`.

## Rozszerzanie systemu

### Dodawanie statystyki

Dwie edycje, obie egzekwowane w czasie kompilacji:

1. dodaj wartość do `StatId` **przed** `Count` (`stats/stat_id.hpp`);
2. dodaj wpis do `kStatNames` (ten sam plik) i do `kStatSpecs`
   (`champions/champion.cpp`), plus parę pól `<stat>` / `<stat>_growth` w
   `ChampionData`.

Obie tablice mają `static_assert` względem `kStatCount`, więc zapomniany wpis
wywala build.

### Dodawanie efektu

Płaski bonus:

```cpp
champion.apply_effect(flat_effect({.source = "Baron", .name = "Hand of Baron"},
                                  {base_mod(StatId::AttackDamage, 20),
                                   base_mod(StatId::Armor, 30)},
                                  Timed::for_span(champion.now(),
                                                  rate.ticks_from_seconds(180.0))));
```

Zależy od innej statystyki — zadeklaruj to, albo `StatView` rzuci:

```cpp
champion.apply_effect(Effect{
    .key = {.source = "Ahri", .name = "Essence Theft"},
    .reads = {StatId::AttackDamage},
    .writes = {StatId::Armor},
    .policy = StackPolicy::Stack,
    .max_stacks = 5,
    .contribute = [](const EffectContext& ctx, ModifierSink& sink) {
        sink.add_base(StatId::Armor, 0.2 * ctx.stats(StatId::AttackDamage));
    },
});
```

Lista kontrolna: czy `reads`/`writes` są kompletne? czy `contribute` jest wolne
od efektów ubocznych? czy `policy` jest tą zamierzoną? czy `key` zgadza się
przy ponownym nałożeniu?

### Dodawanie typu zdarzenia

Nowa para `hpp`/`cpp` w `src/events/` deklarująca przeciążenie `handle_event`,
dodanie typu do wariantu `Event`, dołączenie nagłówka z `event.cpp` i
zarejestrowanie `.cpp` w `src/CMakeLists.txt`.

### Dodawanie pliku źródłowego

Źródła są wymieniane jawnie — bez globowania. Dodaj do `src/CMakeLists.txt`
lub `tests/CMakeLists.txt`, inaczej plik po cichu nie będzie kompilowany. Nix
buduje z drzewa git, więc przed `nix build` / `nix flake check` zrób
`git add` dla nowych plików.

## Strategia testowania

114 testów Catch2. `catch_discover_tests` rejestruje każdy `TEST_CASE` jako
osobny test ctest, więc filtrowanie po nazwie działa przez ctest, a filtrowanie
po tagu wymaga bezpośredniego użycia pliku binarnego:

```sh
ctest --test-dir build                    # wszystko
./build/tests/moba_sim_tests "[effects]"  # po tagu
nix run .#tests -- "[effects]"            # po tagu, bez lokalnego drzewa build
```

| Plik | Pokrywa |
|---|---|
| `stat_pipeline.test.cpp` | algebra wiader w izolacji |
| `stat_table.test.cpp` | wyprowadzanie `StatId`, `StatMask`, `StatTable`, routing modyfikatorów |
| `stat_breakdown.test.cpp` | pochodzenie przez przedmioty, efekty i stacki |
| `tick.test.cpp` | `Tick`/`TickSpan`/`TickRate`, dokładność przy kilku tempach |
| `effect.test.cpp` | czasy życia, wszystkie sześć polityk stackowania, porządkowanie, cykle, czystość |
| `champion.test.cpp` | przyrosty, poziomy, equip/unequip, przetrwanie efektów |
| `champion_effects.test.cpp` | efekty w symulowanym czasie, end-to-end |
| `item.test.cpp` | modyfikatory przedmiotów i etykietowanie |
| `event.test.cpp` | dyspozycja i zagnieżdżanie, aserty na dokładnym wyjściu |
| `game_loop.test.cpp`, `view_geometry.test.cpp` | rytm pętli, geometria |

Konwencje warte utrzymania:

- **Testy muszą być headless.** Testy dotykające SDL same wywołują
  `setenv("SDL_VIDEO_DRIVER", "dummy", 1) — środowisko nie jest ustawiane
  globalnie.
- **Prawdziwe liczby z ręcznie rozpisanymi oczekiwaniami.** Testy mistrzów
  używają rzeczywistych statystyk Ahri z wiki i wypisują arytmetykę w
  komentarzach (`// Level 6 AD: 53 + 3 * 5 = 68`). Test, którego oczekiwanej
  wartości nie da się wyprowadzić ręcznie, niczego nie dokumentuje.
- **Testuj niezmiennik, nie tylko wynik.** „porządek wyznaczania jest niezależny
  od kolejności nakładania”, „powtarzanie `contribute_all` nic nie zmienia”
  i „długa symulacja pozostaje dokładna tick po ticku” wszystkie przeszłyby
  trywialnie na zepsutej implementacji, która przypadkiem dała jedną dobrą
  liczbę.
- **Dokładna równość tam, gdzie deklarowana jest dokładność.** Liczby ticków i
  całkowitoliczbowe wartości statystyk są porównywane przez `==`; tylko
  prawdziwa arytmetyka zmiennoprzecinkowa używa `WithinAbs`.

## Jeszcze niezaimplementowane

Celowo pozostawione luki, w przybliżonej kolejności zależności:

- **Walka.** Brak obrażeń, redukcji, czasu ataku i śmierci. `EffectContext`
  udostępnia tylko statystyki samego efektu — brak celu, właściciela czy innych
  jednostek — więc efekty on-hit, sojusznicze aury i „zyskaj stack za
  zabójstwo” wymagają rozszerzenia tego kontekstu.
- **Umiejętności i cooldowny.** Brak typu `Ability`, kosztów zasobów, czasu
  rzucania.
- **Zdarzenia połączone z symulacją.** `process_event` zwraca `void`, przyjmuje
  tylko strumień i żaden handler nie może dotknąć `Champion`. Nie ma kolejki
  ani harmonogramowania po tickach. Wygasanie efektów już zwraca klucze, co
  jest naturalnym miejscem do rozpoczęcia emitowania zdarzeń.
- **Serializacja i powtórki (replay).** Każda dotychczasowa decyzja projektowa
  utrzymuje to jako możliwe — czas całkowitoliczbowy, klucze semantyczne, czasy
  życia jako dane, deterministyczne porządkowanie — ale nic jeszcze nie zapisuje
  stanu. Uwaga: lambda `contribute` nie jest serializowalna, więc powtórki będą
  wymagały, by *definicje* efektów żyły w rejestrze kluczowanym nazwą, a
  instancje przechowywały tylko klucz i stan runtime'owy.
- **Ścieżki budowania przedmiotów, koszt, unikalne pasywki, limity slotów.**
  `Item` jest dziś płaskim workiem; ten sam przedmiot można założyć dwa razy.
- **Moc umiejętności (ability power).** `StatId` nie ma `AbilityPower`,
  którego potrzebuje większość prawdziwego skalowania.
