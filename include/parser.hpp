#pragma once
#include "parse_table.hpp"
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

class SLRParser {
public:
  SLRParser(const Grammar &g, const SLRParseTable &table)
      : g_(g), table_(table) {}

  bool Parse(const std::vector<SymbolID> &tokens,
             const std::string &label = "") {
    const auto &prods = g_.GetProductions();
    const auto &symbols = g_.GetSymbols();

    auto symName = [&](SymbolID id) -> std::string {
      for (const auto &s : symbols)
        if (s.id == id)
          return s.name;
      return "?";
    };

    std::vector<int> stateStack = {0};
    std::vector<SymbolID> symStack = {}; // symbol pushed with each state (index
                                         // i+1 corresponds to state i for i>0)
    int ip = 0;                          // input pointer

    std::string inputLabel = label.empty() ? "" : label;
    std::cout << "\n=== SLR Parser Trace";
    if (!inputLabel.empty())
      std::cout << ": " << inputLabel;
    std::cout << " ===\n\n";
    std::cout << std::left << std::setw(30) << "Stack (state sym ...)"
              << std::setw(22) << "Input"
              << "Action\n";
    std::cout << std::string(80, '-') << "\n";

    // Build display string for the stack
    auto stackStr = [&]() -> std::string {
      std::string s = "0";
      for (int i = 0; i < (int)symStack.size(); i++) {
        s += " " + symName(symStack[i]) + " " +
             std::to_string(stateStack[i + 1]);
      }
      return s;
    };

    auto inputStr = [&]() -> std::string {
      std::string s;
      for (int i = ip; i < (int)tokens.size(); i++)
        s += symName(tokens[i]) + " ";
      return s;
    };

    auto printRow = [&](const std::string &actionStr) {
      std::cout << std::left << std::setw(30) << stackStr() << std::setw(22)
                << inputStr() << actionStr << "\n";
    };

    // -----------------------------------------------------------------------
    // Parse loop
    // -----------------------------------------------------------------------
    while (true) {
      int state = stateStack.back();
      SymbolID a = tokens[ip];

      auto key = std::make_pair(state, a);
      auto it = table_.action.find(key);

      if (it == table_.action.end()) {
        printRow("ERROR: no action  [state " + std::to_string(state) + ", '" +
                 symName(a) + "']");
        return false;
      }

      const TableCell &cell = it->second;
      const Action &act = cell.primary;

      // Build action description
      std::string actStr = act.ToString(g_);
      if (act.type == ActionType::Shift)
        actStr = "shift " + std::to_string(act.value);
      if (act.type == ActionType::Reduce)
        actStr = "reduce " + act.ToString(g_);
      if (act.type == ActionType::Accept)
        actStr = "ACCEPT";

      if (cell.conflict) {
        actStr += "  *** SHIFT-REDUCE CONFLICT: using primary=[" +
                  act.ToString(g_) + "] over alternative=[" +
                  cell.conflict->ToString(g_) + "] ***";
      }
      printRow(actStr);

      if (act.type == ActionType::Shift) {
        stateStack.push_back(act.value);
        symStack.push_back(a);
        ip++;

      } else if (act.type == ActionType::Reduce) {
        const auto &prod = prods[act.value];
        int popCount = (int)prod.rhs.size();
        for (int i = 0; i < popCount; i++) {
          stateStack.pop_back();
          symStack.pop_back();
        }
        int topState = stateStack.back();
        auto git = table_.gotoT.find({topState, prod.lhs});
        if (git == table_.gotoT.end()) {
          std::cout << "ERROR: no GOTO for state " << topState << " on "
                    << symName(prod.lhs) << "\n";
          return false;
        }
        stateStack.push_back(git->second);
        symStack.push_back(prod.lhs);

      } else if (act.type == ActionType::Accept) {
        std::cout << "\nParsing SUCCEEDED for: " << inputLabel << "\n";
        return true;
      } else {
        return false;
      }
    }
  }

private:
  const Grammar &g_;
  const SLRParseTable &table_;
};
