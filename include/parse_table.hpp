
#pragma once
#include "canonical_lr_collection.hpp"
#include <iomanip>
#include <map>
#include <optional>
#include <string>
#include <vector>

enum class ActionType { Shift, Reduce, Accept, Error };

struct Action {
  ActionType type = ActionType::Error;
  int value = -1; // state index (Shift) or production id (Reduce)

  std::string ToString(const Grammar &g) const {
    const auto &symbols = g.GetSymbols();
    const auto &prods = g.GetProductions();
    auto symName = [&](SymbolID id) -> std::string {
      for (const auto &s : symbols)
        if (s.id == id)
          return s.name;
      return "?";
    };
    switch (type) {
    case ActionType::Shift:
      return "s" + std::to_string(value);
    case ActionType::Accept:
      return "acc";
    case ActionType::Reduce: {
      const auto &p = prods[value];
      std::string r = "r(" + symName(p.lhs) + " -> ";
      for (auto sid : p.rhs)
        r += symName(sid) + " ";
      r += ")";
      return r;
    }
    default:
      return "err";
    }
  }
};

// A cell can have ONE primary action, but we also record any conflict
struct TableCell {
  Action primary;
  std::optional<Action> conflict; // set if there is a shift-reduce conflict
};

class SLRParseTable {
public:
  // action[state][terminalID]
  std::map<std::pair<int, SymbolID>, TableCell> action;
  // gotoT[state][nonTerminalID] = next state
  std::map<std::pair<int, SymbolID>, int> gotoT;

  bool hasConflict = false;

  SLRParseTable(const Grammar &g, const CanonicalLRCollection &col,
                const std::map<SymbolID, std::set<SymbolID>> &follow) {

    const auto &prods = g.GetProductions();
    const auto &symbols = g.GetSymbols();

    auto isTerm = [&](SymbolID id) {
      for (const auto &s : symbols)
        if (s.id == id)
          return s.type == SymbolType::Terminal;
      return false;
    };

    for (int i = 0; i < (int)col.states.size(); i++) {
      for (const auto &item : col.states[i]) {
        const auto &prod = prods[item.prod];

        if (item.dot < (int)prod.rhs.size()) {
          SymbolID X = prod.rhs[item.dot];

          if (isTerm(X)) {
            auto it = col.gotoTable.find({i, X});
            if (it != col.gotoTable.end()) {
              Action shift{ActionType::Shift, it->second};
              auto key = std::make_pair(i, X);
              if (action.count(key)) {
                action[key].conflict = shift;
                hasConflict = true;
              } else {
                action[key] = {shift, {}};
              }
            }
          } else {
            // Non-terminal → GOTO table
            auto it = col.gotoTable.find({i, X});
            if (it != col.gotoTable.end())
              gotoT[{i, X}] = it->second;
          }
        } else {
          if (prod.lhs == 0) {
            // S' -> S .  →  accept on $
            SymbolID dollar = 7;
            Action acc{ActionType::Accept, -1};
            action[{i, dollar}] = {acc, {}};
          } else {
            // Reduce for every terminal in FOLLOW(lhs)
            auto fit = follow.find(prod.lhs);
            if (fit != follow.end()) {
              for (SymbolID a : fit->second) {
                Action red{ActionType::Reduce, prod.id};
                auto key = std::make_pair(i, a);
                if (action.count(key)) {
                  // Shift-reduce (or reduce-reduce) conflict!
                  action[key].conflict = red;
                  hasConflict = true;
                } else {
                  action[key] = {red, {}};
                }
              }
            }
          }
        }
      }

      // GOTO for non-terminals (also filled above, just ensure it's there)
      for (const auto &sym : symbols) {
        if (sym.type != SymbolType::NonTerminal)
          continue;
        auto it = col.gotoTable.find({i, sym.id});
        if (it != col.gotoTable.end())
          gotoT[{i, sym.id}] = it->second;
      }
    }
  }

  // -------------------------------------------------------------------------
  // Pretty-print the full ACTION/GOTO table
  // -------------------------------------------------------------------------
  void Print(const Grammar &g, int numStates) const {
    const auto &symbols = g.GetSymbols();

    // Collect terminal and non-terminal ids in order
    std::vector<SymbolID> terms, nonterms;
    for (const auto &s : symbols) {
      if (s.type == SymbolType::Terminal)
        terms.push_back(s.id);
      else if (s.id != 0)
        nonterms.push_back(s.id); // skip S'
    }

    auto symName = [&](SymbolID id) -> std::string {
      for (const auto &s : symbols)
        if (s.id == id)
          return s.name;
      return "?";
    };

    auto cellStr = [&](int state, SymbolID sym) -> std::string {
      auto key = std::make_pair(state, sym);
      auto it = action.find(key);
      if (it == action.end()) {
        auto git = gotoT.find(key);
        if (git != gotoT.end())
          return std::to_string(git->second);
        return "";
      }
      std::string s = it->second.primary.ToString(g);
      if (it->second.conflict)
        s += "/CONFLICT(" + it->second.conflict->ToString(g) + ")";
      return s;
    };

    // Header
    std::cout << "\n=== SLR Parse Table ===\n";
    std::cout << std::string(6, ' ');
    std::cout << "| ACTION";
    for (size_t i = 0; i < terms.size(); i++)
      std::cout << "           ";
    std::cout << "| GOTO\n";

    std::cout << std::string(6, ' ') << "| ";
    for (auto t : terms)
      std::cout << std::left << std::setw(12) << symName(t);
    std::cout << "| ";
    for (auto nt : nonterms)
      std::cout << std::left << std::setw(6) << symName(nt);
    std::cout << "\n";
    std::cout << std::string(
                     6 + 2 + terms.size() * 12 + 2 + nonterms.size() * 6, '-')
              << "\n";

    for (int i = 0; i < numStates; i++) {
      std::cout << std::left << std::setw(6) << ("I" + std::to_string(i))
                << "| ";
      for (auto t : terms)
        std::cout << std::left << std::setw(12) << cellStr(i, t);
      std::cout << "| ";
      for (auto nt : nonterms)
        std::cout << std::left << std::setw(6) << cellStr(i, nt);
      std::cout << "\n";
    }
  }
};
