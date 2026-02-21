# Adversarial Grid Engine  
## Grid Conflict

### Overview

**Grid Conflict** is a turn-based strategic game played on a discrete rectangular grid.  
Two players (A and B) compete in an adversarial environment where each must manage:

- **H** – Hit Points  
- **A** – Attack Damage  
- **D** – Defense  
- **s** – Current Stamina  
- **S** – Maximum Stamina  

This project implements a decision engine that analyzes a given game state and returns the best move for the current player.

---

### Core Rules

#### Turns and Rounds
- Player **A** starts.
- Players alternate turns.
- A round ends when:
  - the current player runs out of stamina, or
  - the player sends a **pass** move (`p`).

Multiple actions (moves or attacks) may be performed in a single round as long as stamina remains.

---

#### Movement (`m`)
- Moving to an adjacent cell (N/S/E/W) costs **1 stamina**.
- Moving farther costs the **Manhattan distance**.
- Players may pass through entities during movement.
- The final cell must not contain another player or a monster.
- Movement outside the grid is invalid.

---

#### Attack (`a`)
- Costs **10 stamina**.
- Target must be adjacent (N/S/E/W).
- Player vs Player damage:

#### About the project

The core engine was originally developed in C as part of an academic assignment focused on low-level state manipulation and algorithmic game logic. The original version relied on manual memory management, string manipulation, and structured state encoding.