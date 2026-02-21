#include "gcae.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

const int INF = std::numeric_limits<int>::max() / 4;

//
// Helper: map row encoding/decoding:
// 0..25 -> 'A'..'Z', 26..51 -> 'a'..'z'
//
static inline char indexToRowChar(int idx) {
  if (idx < 26)
    return static_cast<char>('A' + idx);
  return static_cast<char>('a' + (idx - 26));
}
static inline int rowCharToIndex(char c) {
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return (c - 'a') + 26;
  return -1;
}

//
// encode: board -> string
// decode: string -> board
//
void encode(int H, int W, const std::vector<std::vector<char>> &board,
            std::string &out) {
  out.clear();
  for (int r = 0; r < H; ++r) {
    for (int c = 0; c < W; ++c) {
      char ch = board[r][c];
      if (!(std::isdigit(static_cast<unsigned char>(ch)) ||
            std::isupper(static_cast<unsigned char>(ch)) ||
            std::islower(static_cast<unsigned char>(ch))))
        continue;
      if (std::isdigit(static_cast<unsigned char>(ch)))
        out.push_back('o');
      out.push_back(ch);
      out.push_back(' ');
      out.push_back(indexToRowChar(r));
      int col = c + 1;
      if (col >= 10)
        out.push_back(char('0' + (col / 10) % 10));
      out.push_back(char('0' + (col % 10)));
      out.push_back(' ');
    }
  }
}

void decode(int H, int W, std::vector<std::vector<char>> &board,
            const std::string &s) {
  for (int i = 0; i < H; ++i)
    board[i].assign(W, '.');

  size_t i = 0, n = s.size();
  while (i < n) {
    // skip spaces
    if (s[i] == ' ') {
      ++i;
      continue;
    }
    bool is_obj = false;
    if (s[i] == 'o') {
      is_obj = true;
      ++i;
      if (i >= n)
        break;
    }
    char ent = s[i++]; // entity char
    // skip optional space
    if (i < n && s[i] == ' ')
      ++i;
    if (i >= n)
      break;
    char rowChar = s[i++];
    int row = rowCharToIndex(rowChar);
    if (row < 0)
      continue;
    if (i >= n)
      break;
    int col = s[i++] - '0';
    if (i < n && std::isdigit(static_cast<unsigned char>(s[i]))) {
      col = col * 10 + (s[i++] - '0');
    }
    if (row >= 0 && row < H && col >= 1 && col <= W) {
      board[row][col - 1] = ent;
    }
    // skip trailing space
    if (i < n && s[i] == ' ')
      ++i;
  }
}

//
// helpers to parse encoded string positions (using int &pos)
//
static inline char getrow(const std::string &str, int &pos) {
  char c = str[pos];
  ++pos;
  return c;
}
static inline int getcol(const std::string &str, int &pos) {
  int col = 0;
  if (pos + 1 < (int)str.size() &&
      std::isdigit(static_cast<unsigned char>(str[pos + 1]))) {
    col = (str[pos++] - '0') * 10;
  }
  col += (str[pos++] - '0');
  return col;
}

//
// string helpers that replace add_c/delete_c/memmove
//
static inline int add_c(std::string &s, int start, int len) {
  if (start < 0 || len <= 0 || start > (int)s.size())
    return -1;
  s.insert((size_t)start, std::string(len, '\0'));
  return 1;
}
static inline int delete_c(std::string &s, int start, int len) {
  if (start < 0 || len <= 0 || start > (int)s.size())
    return -1;
  if (start + len > (int)s.size())
    len = (int)s.size() - start;
  s.erase((size_t)start, (size_t)len);
  return 1;
}

//
// distance (manhattan) using row chars
//
static inline int distance_rc(char row, int col, char row2, int col2) {
  int r1 = rowCharToIndex(row), r2 = rowCharToIndex(row2);
  int vr = (r1 > r2) ? r1 - r2 : r2 - r1;
  int vc = (col > col2) ? col - col2 : col2 - col;
  return vr + vc;
}

//
// closest_point and closest_valid_point
//
static point closest_point(char row_c, int col, char np_row_c, int np_col,
                           int s, int s2) {
  // convert to indices
  int row = rowCharToIndex(row_c);
  int np_row = rowCharToIndex(np_row_c);
  int cur_row = np_row;
  int cur_col = np_col;

  // going down until obstacle
  while (cur_row < row && s) {
    cur_row++;
    --s;
  }
  // going right
  while (cur_col < col && s) {
    ++cur_col;
    --s;
  }
  // going up
  while (cur_row > row && s) {
    --cur_row;
    --s;
  }
  // going left
  while (cur_col > col && s) {
    --cur_col;
    --s;
  }

  // s2 attempts
  while (cur_col < col && s2) {
    ++cur_col;
    --s2;
  }
  while (cur_col > col && s2) {
    --cur_col;
    --s2;
  }
  while (cur_row < row && s2) {
    ++cur_row;
    --s2;
  }
  while (cur_row > row && s2) {
    --cur_row;
    --s2;
  }

  point res;
  res.row = indexToRowChar(cur_row);
  res.col = cur_col;
  return res;
}

static point closest_valid_point(char row, int col, char np_row, int np_col,
                                 int s, const std::string &str, int H, int W) {
  point closest = closest_point(row, col, np_row, np_col, s, 0);
  while (!([&](const point &p) -> bool {
    // valid_position: in map and not already occupied according to encoded
    // string
    int ridx = rowCharToIndex(p.row);
    if (!(ridx >= 0 && ridx < H && p.col > 0 && p.col <= W))
      return false;
    // build simple substr like original
    std::string sub;
    sub.push_back(p.row);
    if (p.col > 9)
      sub.push_back(char('0' + (p.col / 10) % 10));
    sub.push_back(char('0' + (p.col % 10)));
    return (str.find(sub) == std::string::npos);
  })(closest) &&
         s > 0) {
    int s2 = 0;
    while (!([&](const point &p) -> bool {
      int ridx = rowCharToIndex(p.row);
      if (!(ridx >= 0 && ridx < H && p.col > 0 && p.col <= W))
        return false;
      std::string sub;
      sub.push_back(p.row);
      if (p.col > 9)
        sub.push_back(char('0' + (p.col / 10) % 10));
      sub.push_back(char('0' + (p.col % 10)));
      return (str.find(sub) == std::string::npos);
    })(closest) &&
           s2 <= s) {
      closest = closest_point(row, col, np_row, np_col, s - s2, s2);
      ++s2;
    }
    --s;
  }
  return closest;
}

static inline short maxim(short a, short b) { return (a > b) ? a : b; }

static bool isobject_index(int i, const std::string &s) {
  if (i < 0 || i >= (int)s.size())
    return false;
  if (s[i] != 'o')
    return false;
  if (i == 0)
    return true;
  if (i - 2 >= 0) {
    char prev = s[i - 2];
    if (prev == 'm' || prev == 'A' || prev == 'B')
      return false;
  }
  return true;
}

//
// end_round and move_player 
//
static void end_round_state(game_state &dest, const game_state &src, Move &m,
                            int np, int otherp) {
  m.type = 'p';
  m.torow = '.';
  m.tocol = 0;
  dest = src;

}

static bool move_player_apply(game_state &dest, const game_state &src, Move &m,
                              char row, int col, char np_row, int np_col,
                              char next_player) {
  m.type = 'm';
  m.torow = row;
  m.tocol = (short)col;
  dest = src;

  // find player pos in dest.s: find "<next_player><space>"
  std::string key;
  key.push_back(next_player);
  key.push_back(' ');
  size_t pos = dest.s.find(key);
  if (pos == std::string::npos)
    return false;
  pos += 2; // digits start
  // adjust digits length
  if (col > 9) {
    if (pos + 2 > dest.s.size() || dest.s[pos + 2] == ' ') {
      dest.s.insert(pos, 1, '0'); // make space
    }
  } else {
    if (pos + 2 <= dest.s.size() && dest.s[pos + 2] != ' ') {
      dest.s.erase(pos, 1); // shrink
    }
  }
  // write row and col digits
  if (pos >= dest.s.size())
    return false;
  dest.s[pos++] = row;
  if (col > 9) {
    if (pos >= dest.s.size())
      dest.s.push_back('0');
    dest.s[pos++] = char('0' + (col / 10) % 10);
  }
  if (pos >= dest.s.size())
    dest.s.push_back('0');
  dest.s[pos++] = char('0' + (col % 10));
  return true;
}

//
// next_states
//
int next_states(int H, int W, const game_state &gs, char next_player,
                const std::vector<item> &items, std::vector<game_state> &ngs,
                std::vector<Move> &moves) {
  ngs.clear();
  moves.clear();
  // reserve up to 30
  ngs.resize(30);
  moves.resize(30);

  int state = 0;
  int np = (next_player == 'A') ? 0 : 1;
  int otherp = 1 - np;
  char opp = (next_player == 'A') ? 'B' : 'A';

  if (gs.players[np].s == 0) {
    end_round_state(ngs[state], gs, moves[state], np, otherp);
    return 1;
  }

  // find np position in encoded string
  size_t np_pos = gs.s.find(std::string(1, next_player) + " ");
  if (np_pos == std::string::npos) {
    // fallback: end round
    end_round_state(ngs[state], gs, moves[state], np, otherp);
    return 1;
  }
  int tmp = (int)np_pos + 2;
  char np_row = getrow(gs.s, tmp);
  int np_col = getcol(gs.s, tmp);

  // count monsters and objects
  int o = 0, m = 0;
  for (int i = 0; i < (int)gs.s.size(); ++i) {
    if (gs.s[i] == 'm' && i + 1 < (int)gs.s.size() && gs.s[i + 1] == ' ')
      ++m;
    if (isobject_index(i, gs.s))
      ++o;
  }

  // find opponent substring and parse its row/col
  size_t opp_pos = gs.s.find(std::string(1, opp) + " ");
  if (opp_pos == std::string::npos) {
    end_round_state(ngs[state], gs, moves[state], np, otherp);
    return 1;
  }
  tmp = (int)opp_pos + 2;
  char row = getrow(gs.s, tmp);
  int col = getcol(gs.s, tmp);

  int dtop = distance_rc(row, col, np_row, np_col);

  if (dtop == 1) {
    if (gs.players[np].s >= 10) {
      // attack opponent
      Move mm;
      mm.type = 'a';
      mm.torow = row;
      mm.tocol = (short)col;
      ngs[state] = gs;
      // damage
      short damage =
          std::max<short>(0, gs.players[np].A - gs.players[otherp].D);
      ngs[state].players[otherp].H = (short)(gs.players[otherp].H - damage);
      ngs[state].players[np].s = (short)(gs.players[np].s - 10);
      moves[state] = mm;
      ++state;
    } else {
      int di[] = {-1, 1, -1, 1};
      int dj[] = {-1, 1, 1, -1};
      for (int dir = 0; dir < 4; ++dir) {
        char nr = (char)(row + di[dir]);
        int nc = col + dj[dir];
        if (nr == 'Z' + 1)
          nr = 'a';
        if (nr == 'a' - 1)
          nr = 'Z';
        // valid_position check: in encoded string and map bounds
        int ri = rowCharToIndex(nr);
        bool in_bounds = (ri >= 0 && ri < H && nc > 0 && nc <= W);
        bool not_occupied = true;
        if (in_bounds) {
          std::string sub;
          sub.push_back(nr);
          if (nc > 9)
            sub.push_back(char('0' + (nc / 10) % 10));
          sub.push_back(char('0' + (nc % 10)));
          not_occupied = (gs.s.find(sub) == std::string::npos);
        }
        if (in_bounds && not_occupied) {
          int dist = distance_rc(nr, nc, np_row, np_col);
          if (dist <= gs.players[np].s) {
            // create state with move
            Move mm;
            game_state dest = gs;
            if (move_player_apply(dest, gs, mm, nr, nc, np_row, np_col,
                                  next_player)) {
              dest.players[np].s = (short)(gs.players[np].s - dist);
              ngs[state] = std::move(dest);
              mm.type = 'm';
              moves[state] = mm;
              ++state;
              break; // dir = 4
            }
          }
        }
      }
    }
  } else {
    int di[] = {0, 0, 1, -1};
    int dj[] = {1, -1, 0, 0};
    for (int dir = 0; dir < 4; ++dir) {
      char nr = (char)(row + di[dir]);
      int nc = col + dj[dir];
      if (nr == 'Z' + 1)
        nr = 'a';
      if (nr == 'a' - 1)
        nr = 'Z';
      int ri = rowCharToIndex(nr);
      bool in_bounds = (ri >= 0 && ri < H && nc > 0 && nc <= W);
      bool not_occupied = true;
      if (in_bounds) {
        std::string sub;
        sub.push_back(nr);
        if (nc > 9)
          sub.push_back(char('0' + (nc / 10) % 10));
        sub.push_back(char('0' + (nc % 10)));
        not_occupied = (gs.s.find(sub) == std::string::npos);
      }
      if (in_bounds && not_occupied) {
        int dist = distance_rc(nr, nc, np_row, np_col);
        if (dist + 10 <= gs.players[np].s) {
          Move mm;
          game_state dest = gs;
          if (move_player_apply(dest, gs, mm, nr, nc, np_row, np_col,
                                next_player)) {
            dest.players[np].s = (short)(gs.players[np].s - dist);
            ngs[state] = std::move(dest);
            mm.type = 'm';
            moves[state] = mm;
            ++state;
            break;
          }
        } else if (dtop - 1 > gs.players[np].s) {
          point closest = closest_valid_point(nr, nc, np_row, np_col,
                                              gs.players[np].s, gs.s, H, W);
          int d = distance_rc(closest.row, closest.col, np_row, np_col);
          std::string sub;
          sub.push_back(closest.row);
          if (closest.col > 9)
            sub.push_back(char('0' + (closest.col / 10) % 10));
          sub.push_back(char('0' + (closest.col % 10)));
          bool val = (gs.s.find(sub) == std::string::npos);
          if (val && d <= gs.players[np].s) {
            Move mm;
            game_state dest = gs;
            if (move_player_apply(dest, gs, mm, closest.row, closest.col,
                                  np_row, np_col, next_player)) {
              dest.players[np].s = 0;
              ngs[state] = std::move(dest);
              mm.type = 'm';
              moves[state] = mm;
              ++state;
            }
          } else {
            end_round_state(ngs[state], gs, moves[state], np, otherp);
            ++state;
          }
        } else {
          end_round_state(ngs[state], gs, moves[state], np, otherp);
          ++state;
          break;
        }
      }
    }
  }

  // process objects
  // iterate through encoded string, find object prefixes 'o'
  int i = 0;
  while (state <= o + 1) {
    // find next object
    while (i < (int)gs.s.size() && !isobject_index(i, gs.s))
      ++i;
    if (i >= (int)gs.s.size())
      break;
    int nr_o = (int)(gs.s[i + 1] - '0');
    i += 3;
    int start = i - 3, end = i + 1;
    while (end - 1 < (int)gs.s.size() && gs.s[end - 1] != ' ' &&
           gs.s[end - 1] != '\0')
      ++end;
    char orow = getrow(gs.s, i);
    int ocol = getcol(gs.s, i);
    int dist = distance_rc(orow, ocol, np_row, np_col);
    if (dist <= gs.players[np].s) {
      // collect object: delete substring [start, end)
      game_state dest = gs;
      if (delete_c(dest.s, start, end - start) == -1)
        return -1;
      Move mm;
      mm.type = 'm';
      mm.torow = orow;
      mm.tocol = (short)ocol;
      // update player's pos in dest.s
      if (!move_player_apply(dest, gs, mm, orow, ocol, np_row, np_col,
                             next_player))
        return -1;
      // update stats
      dest.players[np].s = (short)(gs.players[np].s - dist);
      dest.players[np].H = (short)(gs.players[np].H + items[nr_o].dH);
      dest.players[np].A = (short)(gs.players[np].A + items[nr_o].dA);
      dest.players[np].D = (short)(gs.players[np].D + items[nr_o].dD);
      dest.players[np].S = (short)(gs.players[np].S + items[nr_o].dS);
      ngs[state] = std::move(dest);
      moves[state] = mm;
    } else {
      point closest = closest_valid_point(orow, ocol, np_row, np_col,
                                          gs.players[np].s, gs.s, H, W);
      std::string sub;
      sub.push_back(closest.row);
      if (closest.col > 9)
        sub.push_back(char('0' + (closest.col / 10) % 10));
      sub.push_back(char('0' + (closest.col % 10)));
      if (gs.s.find(sub) == std::string::npos) {
        Move mm;
        game_state dest = gs;
        if (move_player_apply(dest, gs, mm, closest.row, closest.col, np_row,
                              np_col, next_player)) {
          dest.players[np].s = 0;
          ngs[state] = std::move(dest);
          moves[state] = mm;
        }
      } else {
        end_round_state(ngs[state], gs, moves[state], np, otherp);
      }
    }
    ++state;
  }

  // process monsters
  i = 0;
  while (state <= o + m + 1) {
    // find monster "m "
    while (i < (int)gs.s.size() &&
           !(gs.s[i] == 'm' && i + 1 < (int)gs.s.size() && gs.s[i + 1] == ' '))
      ++i;
    if (i >= (int)gs.s.size())
      break;
    i += 2;
    int start = i - 2, end = i + 1;
    while (end - 1 < (int)gs.s.size() && gs.s[end - 1] != ' ' &&
           gs.s[end - 1] != '\0')
      ++end;
    char mrow = getrow(gs.s, i);
    int mcol = getcol(gs.s, i);
    int dtom = distance_rc(mrow, mcol, np_row, np_col);
    if (dtom == 1 && gs.players[np].s >= 10) {
      // attack monster: remove monster from string
      game_state dest = gs;
      if (delete_c(dest.s, start, end - start) == -1)
        return -1;
      Move mm;
      mm.type = 'a';
      mm.torow = mrow;
      mm.tocol = (short)mcol;
      dest.players[np].s = (short)(gs.players[np].s - 10);
      dest.players[np].H = (short)(gs.players[np].H + 10);
      ngs[state] = std::move(dest);
      moves[state] = mm;
      ++state;
    } else {
      int di[] = {0, 0, 1, -1};
      int dj[] = {1, -1, 0, 0};
      bool progressed = false;
      for (int dir = 0; dir < 4; ++dir) {
        char nr = (char)(mrow + di[dir]);
        int nc = mcol + dj[dir];
        if (nr == 'Z' + 1)
          nr = 'a';
        if (nr == 'a' - 1)
          nr = 'Z';
        int ri = rowCharToIndex(nr);
        bool in_bounds = (ri >= 0 && ri < H && nc > 0 && nc <= W);
        std::string sub;
        sub.push_back(nr);
        if (nc > 9)
          sub.push_back(char('0' + (nc / 10) % 10));
        sub.push_back(char('0' + (nc % 10)));
        if (in_bounds && gs.s.find(sub) == std::string::npos) {
          int dist = distance_rc(nr, nc, np_row, np_col);
          if (dist <= gs.players[np].s) {
            Move mm;
            game_state dest = gs;
            if (move_player_apply(dest, gs, mm, nr, nc, np_row, np_col,
                                  next_player)) {
              dest.players[np].s = (short)(gs.players[np].s - dist);
              ngs[state] = std::move(dest);
              moves[state] = mm;
              ++state;
              progressed = true;
              break;
            }
          } else if (dtom > gs.players[np].s) {
            point closest = closest_valid_point(nr, nc, np_row, np_col,
                                                gs.players[np].s, gs.s, H, W);
            std::string subs;
            subs.push_back(closest.row);
            if (closest.col > 9)
              subs.push_back(char('0' + (closest.col / 10) % 10));
            subs.push_back(char('0' + (closest.col % 10)));
            if (gs.s.find(subs) == std::string::npos) {
              Move mm;
              game_state dest = gs;
              if (move_player_apply(dest, gs, mm, closest.row, closest.col,
                                    np_row, np_col, next_player)) {
                dest.players[np].s = 0;
                ngs[state] = std::move(dest);
                moves[state] = mm;
              }
            } else {
              end_round_state(ngs[state], gs, moves[state], np, otherp);
            }
            ++state;
            progressed = true;
            break;
          }
        }
      }
      if (!progressed) {
        // nothing found: continue
      }
    }
  }

  // final end_round
  end_round_state(ngs[state], gs, moves[state], np, otherp);
  ++state;
  ngs.resize(state);
  moves.resize(state);
  return state;
}

//
// static_eval and game_over
//
static inline bool game_over_check(const game_state &gs) {
  return gs.players[0].H <= 0 || gs.players[1].H <= 0;
}

static int static_eval(const game_state &gs, char next_player) {
  int player_index = (next_player == 'A') ? 0 : 1;
  int opp_index = 1 - player_index;
  if (game_over_check(gs) && gs.players[player_index].H > 0)
    return INF;
  if (game_over_check(gs) && gs.players[player_index].H <= 0)
    return -INF;
  int score = (int)gs.players[player_index].H + gs.players[player_index].A +
              gs.players[player_index].D + gs.players[player_index].S -
              ((int)gs.players[opp_index].H + gs.players[opp_index].A +
               gs.players[opp_index].D + gs.players[opp_index].S);
  return score;
}

//
// minimax with alpha-beta; depth-limited; uses next_states
//
static int minimax_search(const game_state &gs, int depth, bool is_maximizing,
                          int H, int W, char next_player,
                          const std::vector<item> &items, int &alpha,
                          int &beta) {
  if (depth == 0 || game_over_check(gs))
    return static_eval(gs, next_player);

  std::vector<game_state> ngs;
  std::vector<Move> moves;
  char cur = is_maximizing ? next_player : (next_player == 'A' ? 'B' : 'A');

  int nos = next_states(H, W, gs, cur, items, ngs, moves);
  if (is_maximizing) {
    int max_eval = -INF;
    for (int i = 0; i < nos; ++i) {
      int eval;
      int player_index = (next_player == 'A') ? 0 : 1;
      if (moves[i].type == 'p' || ngs[i].players[player_index].s == 0) {
        eval = minimax_search(ngs[i], depth - 1, false, H, W, next_player,
                              items, alpha, beta);
      } else {
        eval = minimax_search(ngs[i], depth, true, H, W, next_player, items,
                              alpha, beta);
      }
      if (eval > max_eval)
        max_eval = eval;
      if (eval > alpha)
        alpha = eval;
      if (beta <= alpha)
        break;
    }
    return max_eval;
  } else {
    int min_eval = INF;
    for (int i = 0; i < nos; ++i) {
      int eval;
      int opp_index = (next_player == 'A') ? 1 : 0;
      if (moves[i].type == 'p' || ngs[i].players[opp_index].s == 0) {
        eval = minimax_search(ngs[i], depth - 1, true, H, W, next_player, items,
                              alpha, beta);
      } else {
        eval = minimax_search(ngs[i], depth, false, H, W, next_player, items,
                              alpha, beta);
      }
      if (eval < min_eval)
        min_eval = eval;
      if (eval < beta)
        beta = eval;
      if (beta <= alpha)
        break;
    }
    return min_eval;
  }
}

//
// best_move: parse file, run next_states + minimax, return best Move
//
Move best_move(const char *file_name) {
  Move nullm;
  nullm.type = 'p';
  nullm.torow = '.';
  nullm.tocol = 0;
  std::ifstream fin(file_name);
  if (!fin.is_open())
    return nullm;

  int H, W;
  char next_player;
  fin >> H >> W >> next_player;
  if (!fin)
    return nullm;

  game_state gs;
  fin >> gs.players[0].H >> gs.players[0].A >> gs.players[0].D >>
      gs.players[0].s >> gs.players[0].S;
  fin >> gs.players[1].H >> gs.players[1].A >> gs.players[1].D >>
      gs.players[1].s >> gs.players[1].S;

  int n;
  fin >> n;
  std::vector<item> items;
  items.resize(std::max(0, n));
  for (int i = 0; i < n; ++i) {
    fin >> items[i].dH >> items[i].dA >> items[i].dD >> items[i].dS;
  }
  std::string rest;
  std::getline(fin, rest); // eat endline
  std::getline(fin, gs.s);
  if (!gs.s.empty() && gs.s.back() == '\r')
    gs.s.pop_back();

  // decode for debug (optional)
  // create board and print (optional)
  // compute next states
  std::vector<game_state> ngs;
  std::vector<Move> moves;
  int nr_of_states = next_states(H, W, gs, next_player, items, ngs, moves);
  if (nr_of_states <= 0)
    return nullm;

  int best_index = 0;
  int best_score = -INF;
  int alpha = -INF, beta = INF;
  int depth = 15; // realistic depth
  for (int i = 0; i < nr_of_states; ++i) {
    bool is_maximizing =
        (moves[i].type == 'p') ? false : true;
    int a = alpha, b = beta;
    int score = minimax_search(ngs[i], depth, is_maximizing, H, W, next_player,
                               items, a, b);
    if (score > best_score) {
      best_score = score;
      best_index = i;
    }
  }

  return moves[best_index];
}
