# Melody Player – Bluetooth riadený hudobno-svetelný modul

## Prehľad projektu
Tento projekt implementuje vnorený hudobno-svetelný modul. Zariadenie umožňuje prehrávanie melódií a riadenie LED panelu prostredníctvom bezdrôtovej komunikácie cez Bluetooth. Používateľ má možnosť prepínať režimy, voliť melódie a ovplyvňovať správanie svetelnej vizualizácie.

---

## Funkcionalita

### Režimy
Systém pracuje s viacerými prevádzkovými režimami, ktoré definujú globálne správanie zariadenia.

Režimy určujú:
- spôsob prehrávania melódií (jedna melódia, sekvencia, automatické prepínanie),
- aktivitu a logiku LED vizualizácie,
- reakciu systému na príkazy prijaté cez Bluetooth.

Režimy je možné meniť počas behu systému bez nutnosti reštartu.

---

### Melódie
Melódie sú reprezentované ako postupnosti tónov s definovanou frekvenciou a dĺžkou trvania.

Podporované vlastnosti:
- výber konkrétnej melódie,
- prehrávanie v slučke,
- automatická zmena melódií v závislosti od zvoleného režimu.

Zvukový výstup je realizovaný pomocou PWM signálu generovaného mikrokontrolérom.

---

### LED vizualizácia
Svetelný výstup je založený na adresovateľnom RGB LED paneli WS2812B.

Charakteristiky:
- LED vzory riadia poradie a topológiu zapínania LED,
- farby LED môžu byť odvodené od frekvencie alebo rytmu prehrávaných tónov,
- svetelná časť pracuje synchronizovane so zvukovým výstupom.

---

## Komunikácia a ovládanie

### Bluetooth komunikácia
Bezdrôtová komunikácia je realizovaná pomocou Bluetooth modulu HC-05.
Zariadenie funguje ako slave jednotka, ktorá prijíma príkazy z externého zariadenia.

---

### Ovládací model
Príkazy prijaté cez Bluetooth umožňujú:
- výber režimu,
- výber melódie,
- spustenie alebo zastavenie prehrávania,
- zmenu správania LED vizualizácie.

Spracovanie príkazov prebieha v reálnom čase alebo podľa aktívneho režimu systému.

---

## Hardvérové zapojenie systému

Táto kapitola popisuje fyzické zapojenie jednotlivých komponentov systému podľa priloženej schémy. Zapojenie je navrhnuté tak, aby bolo možné paralelne prehrávať zvuk, ovládať LED panel a komunikovať cez Bluetooth pri spoločnom napájaní a zdieľanej zemi.

---

### Napájanie
Systém je napájaný **5 V zdrojom** privedeným cez USB konektor.

- **VBUS (5 V)** je rozvedený na napájacie lišty breadboardu
- **GND** je spoločná zem pre:
  - mikrokontrolér STM32,
  - LED panel WS2812B,
  - Bluetooth modul HC-05,
  - výkonový stupeň reproduktora

Použitie spoločnej zeme je nutné pre správnu funkciu komunikácie a časovania signálov.

---

### Mikrokontrolér STM32
STM32 slúži ako centrálna riadiaca jednotka systému.

Zabezpečuje:
- generovanie PWM signálu pre reproduktor,
- riadenie dátovej linky LED panelu,
- UART komunikáciu s Bluetooth modulom.

Mikrokontrolér je osadený na breadboarde a prepojený s ostatnými komponentmi pomocou prepojovacích vodičov.

---

### LED panel WS2812B (8×8)
Adresovateľný RGB LED panel je pripojený troma vodičmi:

- **VCC (5 V)** – napájanie z hlavnej 5 V vetvy,
- **GND** – spoločná zem,
- **DATA** – dátový signál z GPIO pinu STM32.

LED panel používa jednosmernú dátovú komunikáciu, kde STM32 generuje presne časovaný signál pre riadenie jednotlivých LED.

---

### Bluetooth modul HC-05
Bluetooth modul umožňuje bezdrôtové ovládanie systému.

Zapojenie:
- **VCC** → 5 V napájanie od STM32,
- **GND** → spoločná zem,
- **TXD** → RX pin STM32 (UART),
- **RXD** → TX pin STM32 (UART).

Komunikácia prebieha ako klasická sériová linka UART. Modul pracuje v slave režime a prijíma príkazy z externého zariadenia.

---

### Reproduktor
Reproduktor je ovládaný nepriamo cez tranzistor, ktorý slúži ako výkonový spínač.

Zapojenie:
- PWM výstup STM32 je privedený na bázu tranzistora,
- tranzistor spína prúd tečúci cez reproduktor,
- reproduktor je napájaný z 5 V linky,
- zem výkonového stupňa je spoločná so zemou systému.

Medzi napájanie a zem sú pridané **odrušovacie kondenzátory (100 nF)**, ktoré znižujú špičky a rušenie spôsobené spínaním záťaže.

---

### Odrušenie a stabilita
Kondenzátory pripojené medzi **5 V a GND**:
- stabilizujú napájacie napätie,
- znižujú elektromagnetické rušenie,
- zlepšujú spoľahlivosť Bluetooth komunikácie a LED signálu.

---

### Zhrnutie zapojenia
- jeden spoločný 5 V napájací zdroj,
- jedna spoločná zem pre všetky moduly,
- UART pre Bluetooth komunikáciu,
- PWM pre generovanie zvuku,
- jednovodičová dátová linka pre LED panel.

Zapojenie je navrhnuté ako jednoduché, prehľadné a vhodné na testovanie aj demonštráciu funkčnosti systému.


---

## Štruktúra projektu

Projekt je organizovaný podľa štandardnej štruktúry:

### Core/Inc
Obsahuje hlavičkové súbory definujúce verejné rozhrania jednotlivých modulov:
- `bt.h` – rozhranie Bluetooth komunikácie,
- `melodies.h` – definície melódií a ich identifikátorov,
- `speaker.h` – rozhranie zvukového výstupu,
- `ws2812.h` – rozhranie LED panelu,
- `main.h` – globálne definície projektu,
- systémové hlavičky STM32 HAL a prerušenia.

---

### Core/Src
Obsahuje implementáciu jednotlivých funkčných blokov:
- `bt.c` – spracovanie Bluetooth komunikácie a príkazov,
- `melodies.c` – definície a správa melódií,
- `speaker.c` – generovanie zvuku pomocou PWM,
- `ws2812.c` – riadenie adresovateľného LED panelu,
- `main.c` – inicializácia systému a hlavná riadiaca slučka,
- systémové súbory STM32 (HAL, prerušenia, štart systému).

---
