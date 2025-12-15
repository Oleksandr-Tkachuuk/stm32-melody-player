# Technická špecifikácia projektu
## STM32 – Prehrávanie melódií s LED vizualizáciou a Bluetooth ovládaním
Version: 1.0  
Date: 2025

---

# 1. PREHĽAD PROJEKTU

## 1.1 Cieľ
Cieľom projektu je navrhnúť a implementovať vstavaný systém založený na **STM32F303K8**, ktorý umožňuje:

- prehrávať melódie pomocou PWM cez pasívny reproduktor,
- zobrazovať vizualizáciu prehrávanej melódie na 8-segmentovej RGB LED,
- ovládať zariadenie bezdrôtovo pomocou Bluetooth.

Systém má byť modulárny, spoľahlivý a vhodný na prezentáciu princípov embedded systémov.

---

## 1.2 Funkcie systému

### Hlavné funkcie
- Prehrávanie melódií (uložených v tabuľkách tónov)
- PWM generovanie zvukového signálu
- RGB LED vizualizácie viazané na tóny alebo tempo
- Ovládanie cez Bluetooth:
  - spustenie / zastavenie prehrávania
  - výber melódie
  - prepínanie LED režimov

### Podfunkcie
- Jednoduchý UART protokol
- Ošetrenie chýb pri komunikácii
- Konfigurovateľné farby pre LED animácie
- Stabilná časová základňa pre tóny (TIM)

---

# 2. SYSTÉMOVÁ ARCHITEKTÚRA

## 2.1 Blokový diagram

```text
Bluetooth modul
        │ UART
        ▼
┌────────────────────────────┐
│        STM32F303K8         │
│                            │
│   ┌──────────────┐         │
│   │  PWM Timer   │ ──> Speaker (pasívny)
│   └──────────────┘         │
│                            │
│   ┌──────────────┐         │
│   │ LED Driver   │ ──> RGB LED (8 segmentov)
│   └──────────────┘         │
│                            │
│   ┌──────────────┐         │
│   │ UART Handler │ ──> Bluetooth
│   └──────────────┘         │
└────────────────────────────┘
```

## 2.2 Tok dát

- **Bluetooth → STM32**  
  Prijímanie príkazov: `START`, `STOP`, `SET <melody_id>`, `MODE <id>`

- **STM32 → periférie**
  - PWM timer generuje zvuk
  - LED driver generuje farby podľa aktuálneho tónu

- **STM32 → Bluetooth**
  - odozvy systému: `OK`, `ERR`, `PLAYING`, `STOPPED`

---

## 3. HARDVÉROVÁ ŠPECIFIKÁCIA

### 3.1 Komponenty

- **MCU:** STM32 Nucleo STM32F303K8
- **Audio výstup:** pasívny bzučiak alebo 8 Ω reproduktor  
  - ovládanie pomocou NPN tranzistora (napr. 2N2222)
- **RGB LED:** WS2812B  
  - https://techfun.sk/produkt/rgb-led-matrix-5x5-ws2812b-25-bit/
- **Bluetooth modul:** HC-05 / HC-06  
  - https://techfun.sk/produkt/bluetooth-modul-hc-05-slavemaster/
- **Prototypovanie:** breadboard + jumper kábliky


---

# 4. SOFTVÉROVÁ ARCHITEKTÚRA

## 4.1 Moduly firmware

### Modul 1 (Bozhenkov): Audio PWM generátor
- TIMx nastavený na variabilnú frekvenciu podľa tabuliek tónov  
- Funkcie:
  - `audio_init()`
  - `audio_play_note()`
  - `audio_stop()`

### Modul 2 (Tkachuk): LED vizualizácie
- Nastavovanie farieb podľa tónu alebo rytmu  
- Režimy:
  - statické farby
  - pulzovanie
  - dynamické prechody
- Funkcie:
  - `led_set_mode(id)`
  - `led_update()`

### Modul 3 (Lobach): Bluetooth protokol
- UART RX + TX  
- Parsovanie textových príkazov:  
  - `START`, `STOP`, `SET 0..N`, `MODE 0..N`  
- Funkcie:
  - `bt_init()`
  - `bt_process_rx()`
  - `bt_send_status()`

### Modul 4 (Zamolotniev): Scheduler a hlavný cyklus
- Periodické obnovovanie LED animácií
- Časovanie dĺžky tónov
- Stavový automat (STATE_STOPPED, STATE_PLAYING, STATE_PAUSED)

---

# 5. PLÁN IMPLEMENTÁCIE

1. Príprava projektu v STM32CubeIDE
2. Implementácia PWM generátora tónov
3. Implementácia tabuliek melódií
4. LED driver + animácie
5. Bluetooth modul + príkazový parser
6. Integrácia do hlavného slučky
7. Testovanie na hardvéri

---

# 6. TESTOVANIE

## 6.1 Testovacie scenáre
- Prehrávanie jednej melódie
- Zmena melódie počas prehrávania
- STOP počas prehrávania
- Prepínanie LED režimov
- Nesprávne príkazy (validácia, ERR)

---

