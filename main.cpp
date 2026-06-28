#include "include/canonical_lr_collection.hpp"
#include "include/grammar.hpp"
#include "include/parse_table.hpp"
#include "include/parser.hpp"
#include <iostream>

int main() {
  Grammar g;

  // -------------------------------------------------------------------------
  // 1. FIRST and FOLLOW
  // -------------------------------------------------------------------------
  std::cout << "============================================================\n";
  std::cout << "  SLR PARSER — Project C\n";
  std::cout
      << "============================================================\n\n";

  std::cout << "Grammar productions:\n";
  std::cout << "  0: S' -> S\n";
  std::cout << "  1: S  -> L = R\n";
  std::cout << "  2: S  -> R\n";
  std::cout << "  3: L  -> * R\n";
  std::cout << "  4: L  -> id\n";
  std::cout << "  5: R  -> L\n\n";

  auto first = g.ComputeFirst();
  auto follow = g.ComputeFollow(first);

  g.PrintSet(first, "FIRST sets");
  std::cout << "\n";
  g.PrintSet(follow, "FOLLOW sets");
  std::cout << "\n";

  // -------------------------------------------------------------------------
  // 2. Canonical LR(0) collection
  // -------------------------------------------------------------------------
  std::cout << "============================================================\n";
  std::cout << "  Canonical LR(0) Item Collection\n";
  std::cout
      << "============================================================\n\n";
  CanonicalLRCollection col(g);
  col.Print(g);

  // -------------------------------------------------------------------------
  // 3. SLR Parse Table
  // -------------------------------------------------------------------------
  std::cout
      << "\n============================================================\n";
  std::cout << "  SLR(1) Parse Table\n";
  std::cout << "============================================================\n";
  SLRParseTable table(g, col, follow);
  table.Print(g, (int)col.states.size());

  if (table.hasConflict) {
    std::cout << "\n*** SHIFT-REDUCE CONFLICT detected in the table! ***\n";
    std::cout << "  Location: ACTION[I2, =]\n";
    std::cout << "  State I2 contains:\n";
    std::cout << "    [S -> L . = R]   => shift '=', go to I6\n";
    std::cout << "    [R -> L .]       => reduce R->L for each a in "
                 "FOLLOW(R)={=,$}\n";
    std::cout
        << "  Both shift(6) and reduce(R->L) are valid for '=' => CONFLICT\n";
    std::cout << "  SLR resolves by preferring SHIFT (standard convention).\n";
  }

  // -------------------------------------------------------------------------
  // 4a. Parse  id = id * id
  //     This string is NOT in the language of this grammar because after
  //     L = , the only valid 'R' values are:  * id,  id,  * * id, etc.
  //     'id * id' is not a valid R.  The purpose is to trigger the
  //     shift-reduce conflict at state I2 with lookahead '='.
  //
  //     Token IDs: id=6, ==5, *=4, $=7
  // -------------------------------------------------------------------------
  std::cout
      << "\n============================================================\n";
  std::cout << "  Test 1: id = id * id  (conflict demonstration)\n";
  std::cout << "============================================================\n";
  std::cout << "NOTE: The string 'id = id * id' exposes the S/R conflict\n";
  std::cout << "at state I2 (lookahead '=') and then errors because\n";
  std::cout << "'id * id' is not a valid R in this grammar.\n";

  // id = id * id $   →  6 5 6 4 6 7
  std::vector<SymbolID> input1 = {6, 5, 6, 4, 6, 7};
  SLRParser parser(g, table);
  parser.Parse(input1, "id = id * id");

  // -------------------------------------------------------------------------
  // 4b. Parse  id = * id  (valid string, conflict resolved by shift)
  //     id = * id  →  6 5 4 6 7
  // -------------------------------------------------------------------------
  std::cout
      << "\n============================================================\n";
  std::cout << "  Test 2: id = * id  (valid string, conflict resolved)\n";
  std::cout << "============================================================\n";
  std::cout << "NOTE: 'id = * id' IS in the language (L = *R = *(id)).\n";
  std::cout << "The S/R conflict in I2 is resolved by SHIFT on '=',\n";
  std::cout << "which is the correct parse.\n";

  // id = * id $   →  6 5 4 6 7
  std::vector<SymbolID> input2 = {6, 5, 4, 6, 7};
  parser.Parse(input2, "id = * id");

  return 0;
}
