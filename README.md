# Adversarial Grid Engine
## Grid Conflict

Repository Description
A turn-based adversarial grid game engine that analyzes encoded game states and computes optimal moves using minimax with alpha-beta pruning.

Overview

Grid Conflict is a turn-based strategy game played on a discrete rectangular grid. Two players (A and B) compete in an adversarial environment where each manages:

- H – Hit Points
- A – Attack Damage
- D – Defense
- s – Current Stamina
- S – Maximum Stamina

The engine evaluates a given game state and determines the optimal move for the current player.

Core Rules

Turns and Rounds
- Player A starts.
- Players alternate turns.
- A round ends when:
  - the current player runs out of stamina, or
  - the player issues a pass move (type 'p').
- Multiple actions may be performed within the same round if stamina allows.

Movement (type 'm')
- Moving to an adjacent cell (N/S/E/W) costs 1 stamina.
- Moving farther costs the Manhattan distance.
- Entities can be passed through during movement.
- The final position cannot contain another player or a monster.
- Movement outside the grid is invalid.

Attack (type 'a')
- Costs 10 stamina.
- Target must be adjacent (N/S/E/W).
- Player vs Player damage:
  max(0, attacker.A - defender.D)
- Player vs Monster:
  - Monster is defeated instantly.
  - Player gains +10 HP.

Item Collection
- The first player reaching an item collects it.
- The item disappears from the board.
- Player stats are modified by:
  - dH (HP change)
  - dA (Attack change)
  - dD (Defense change)
  - dS (Max stamina change)
- Values may be negative.
- Current stamina is unaffected.

Win Conditions

- A player loses immediately if H <= 0.
- If 10 rounds are completed, compute:
  (HA + AA + DA + SA) - (HB + AB + DB + SB)
  - If > 0, Player A wins.
  - If <= 0, Player B wins.

Engine Functionality

The engine performs the following steps:

1. Reads a complete game state from a text file.
2. Decodes the encoded board representation.
3. Generates all valid next states.
4. Evaluates states using a static evaluation function.
5. Applies minimax with alpha-beta pruning.
6. Returns the best move via:

   Move best_move(const char* file_name);

The engine operates strictly on the current state and does not store historical moves.

Input File Format

H W next_player
AH AA AD As AS
BH BA BD Bs BS
n
dH0 dA0 dD0 dS0
...
dHn-1 dAn-1 dDn-1 dSn-1
s

Where:
- H, W are grid dimensions (H <= 52, W < 100)
- next_player is 'A' or 'B'
- Player stats follow
- n is the number of items (0–10)
- Item modifiers follow
- s is the encoded board state

The board is NOT written as a visual matrix. Only the encoded string representation is provided.

Example Encoded State (3x3 Board)

Visual board:
A m m
m m m
m m B

Encoded string:
A A1 m A2 m A3 m B1 m B2 m B3 m C1 m C2 B C3

Order of elements does not matter.

Implementation Background

The engine was originally developed in C as part of an academic assignment focused on low-level state encoding, structured data manipulation, and adversarial decision logic.

The current version is a modernized C++ adaptation that preserves the original algorithms and game logic while improving structure, safety, and maintainability. Manual memory management and raw string operations were replaced with std::string and std::vector, and the system was reorganized into clearer components: state representation, move generation, evaluation, and search.

The underlying decision-making strategy remains identical to the original C implementation.

Project Structure

- poe.cpp   – Engine implementation
- poe.hpp   – Data structures and declarations
- main.cpp  – Runner for testing

Build

g++ -std=c++17 -O2 poe.cpp main.cpp -o grid_conflict

Run

./grid_conflict input.txt

Output format:

type torow tocol

Examples:
m A 3
a B 2
p . 0

Project Name
Adversarial Grid Engine
Game Title: Grid Conflict