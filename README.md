# Adversarial Grid Engine  
## Grid Conflict

A turn-based adversarial grid engine that reads an encoded game state and computes the optimal move using Minimax with Alpha-Beta pruning.

---

## Overview

**Grid Conflict** is a two-player strategy game played on a rectangular grid.

Each player (A and B) has the following attributes:

- **H** – Hit Points  
- **A** – Attack Damage  
- **D** – Defense  
- **s** – Current Stamina  
- **S** – Maximum Stamina  

The engine evaluates the current state and returns the best move for the active player.

---

## Core Mechanics

### Turn Structure

- Player A starts.
- Players alternate turns.
- A round ends when:
  - The current player runs out of stamina, or
  - The player issues a pass move (`p`).
- Multiple actions may be performed within the same round if stamina allows.

---

### Movement (`m`)

- Moving to an adjacent cell (N/S/E/W) costs **1 stamina**.
- Moving farther costs the **Manhattan distance**.
- Entities can be passed through during movement.
- The final position must be empty (no player or monster).
- Movement outside the grid is invalid.

---

### Attack (`a`)

- Costs **10 stamina**.
- Target must be adjacent (N/S/E/W).

Damage rules:

- **Player vs Player**
  
  ```
  max(0, attacker.A - defender.D)
  ```

- **Player vs Monster**
  - Monster is defeated instantly.
  - Player gains +10 HP.

---

### Item Collection

- The first player reaching an item collects it.
- The item disappears from the board.
- Item modifiers affect player stats:

  - `dH` – HP change  
  - `dA` – Attack change  
  - `dD` – Defense change  
  - `dS` – Max stamina change  

- Values may be negative.
- Current stamina remains unchanged.

---

## Win Conditions

- A player loses immediately if `H <= 0`.

- If 10 rounds are completed, compute:

  ```
  (HA + AA + DA + SA) - (HB + AB + DB + SB)
  ```

  - If result > 0 → Player A wins.
  - If result <= 0 → Player B wins.

---

## Engine Functionality

The engine:

1. Reads a complete game state from a text file.
2. Decodes the encoded board representation.
3. Generates all valid next states.
4. Evaluates states using a static evaluation function.
5. Applies Minimax with Alpha-Beta pruning.
6. Returns the best move via:

   ```cpp
   Move best_move(const char* file_name);
   ```

The engine operates strictly on the current state and does not store historical moves.

---

## Input File Format

```
H W next_player
AH AA AD As AS
BH BA BD Bs BS
n
dH0 dA0 dD0 dS0
...
dHn-1 dAn-1 dDn-1 dSn-1
s
```

Where:

- `H`, `W` – grid dimensions (H ≤ 52, W < 100)
- `next_player` – `'A'` or `'B'`
- Player stats follow
- `n` – number of items (0–10)
- Item modifiers follow
- `s` – encoded board state string

The board is **not** written as a visual matrix. Only the encoded string representation is provided.

---

## Example Encoded State (3x3 Board)

Visual representation:

```
A m m
m m m
m m B
```

Encoded string:

```
A A1 m A2 m A3 m B1 m B2 m B3 m C1 m C2 B C3
```

Order of elements does not matter.

---

## Implementation Background

The engine was originally developed in C as part of an academic assignment focused on:

- Low-level state encoding
- Structured data manipulation
- Adversarial decision logic

The current version is a modernized C++ adaptation that preserves the original algorithms and game logic while improving structure, safety, and maintainability.

Key improvements:

- Manual memory management replaced with `std::string` and `std::vector`
- Clear separation of:
  - State representation
  - Move generation
  - Evaluation
  - Search logic

The decision-making strategy remains identical to the original C implementation.

---

## Project Structure

```
age.cpp    – Engine implementation  
age.hpp    – Data structures and declarations  
main.cpp   – Runner / entry point  
input.txt  – Example input state  
```

---

## Build

You must compile both `age.cpp` and `main.cpp`.

```
g++ -std=c++17 -O2 age.cpp main.cpp -o grid_conflict
```

If using Windows (MinGW):

```
g++ -std=c++17 -O2 age.cpp main.cpp -o grid_conflict.exe
```

---

## Run

```
./grid_conflict input.txt
```

Windows:

```
grid_conflict.exe input.txt
```

## Run

```
./grid_conflict input.txt
```

---

## Output Format

```
type torow tocol
```

Examples:

```
m A 3   -> Move to cell A3  
a B 2   -> Attack entity at B2  
p . 0   -> End the current round  
```

---

## Project Information

**Project Name:** Adversarial Grid Engine  
**Game Title:** Grid Conflict
