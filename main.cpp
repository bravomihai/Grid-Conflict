#include "poe.hpp"
#include <iostream>

int main(int argc, char **argv) {
  const char *fname = "input.txt";
  if (argc >= 2)
    fname = argv[1];

  Move mv = best_move(fname);

  // Print a compact representation:
  // For example:
  // m A 3    -> move to A3
  // a b 10   -> attack on b10
  // p . 0    -> pass / end round
  std::cout << mv.type << ' ' << mv.torow << ' ' << mv.tocol << '\n';
  return 0;
}
