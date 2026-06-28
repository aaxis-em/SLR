#pragma once
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using SymbolID = int;
using ProductionID = int;

enum class SymbolType { Terminal, NonTerminal };

struct Symbol {
  SymbolID id;
  std::string name;
  SymbolType type;
};

struct Production {
  ProductionID id;
  SymbolID lhs;
  std::vector<SymbolID> rhs;
};

class Grammar {
private:
  std::vector<Symbol> symbols;
  std::vector<Production> grammar;

public:
  Grammar() {
    // Non-terminals
    symbols.push_back({0, "S'", SymbolType::NonTerminal});
    symbols.push_back({1, "S", SymbolType::NonTerminal});
    symbols.push_back({2, "L", SymbolType::NonTerminal});
    symbols.push_back({3, "R", SymbolType::NonTerminal});

    // Terminals
    symbols.push_back({4, "*", SymbolType::Terminal});
    symbols.push_back({5, "=", SymbolType::Terminal});
    symbols.push_back({6, "id", SymbolType::Terminal});
    symbols.push_back({7, "$", SymbolType::Terminal}); // EOF

    grammar.push_back({0, 0, {1}});
    grammar.push_back({1, 1, {2, 5, 3}});
    grammar.push_back({2, 1, {3}});
    grammar.push_back({3, 2, {4, 3}});
    grammar.push_back({4, 2, {6}});
    grammar.push_back({5, 3, {2}});
  }

  const std::vector<Production> &GetProductions() const { return grammar; }

  const std::vector<Symbol> &GetSymbols() const { return symbols; }

  //----------------------------------------
  // FIRST
  //----------------------------------------

  std::map<SymbolID, std::set<SymbolID>> ComputeFirst() const {
    std::map<SymbolID, std::set<SymbolID>> first;

    // FIRST(terminal)=terminal
    for (const auto &s : symbols) {
      if (s.type == SymbolType::Terminal)
        first[s.id].insert(s.id);
    }

    bool changed = true;

    while (changed) {
      changed = false;

      for (const auto &prod : grammar) {
        if (prod.rhs.empty())
          continue;

        SymbolID X = prod.rhs.front();

        for (auto terminal : first[X]) {
          if (first[prod.lhs].insert(terminal).second)
            changed = true;
        }
      }
    }

    return first;
  }

  //----------------------------------------
  // FOLLOW
  //----------------------------------------

  std::map<SymbolID, std::set<SymbolID>>
  ComputeFollow(const std::map<SymbolID, std::set<SymbolID>> &first) const {

    std::map<SymbolID, std::set<SymbolID>> follow;

    // FOLLOW(start)={$}
    follow[0].insert(7);

    bool changed = true;

    while (changed) {
      changed = false;

      for (const auto &prod : grammar) {
        for (size_t i = 0; i < prod.rhs.size(); i++) {
          SymbolID B = prod.rhs[i];

          if (symbols[B].type != SymbolType::NonTerminal)
            continue;

          //--------------------------------
          // B is last symbol
          //--------------------------------
          if (i + 1 == prod.rhs.size()) {
            for (auto x : follow[prod.lhs]) {
              if (follow[B].insert(x).second)
                changed = true;
            }
          }
          //--------------------------------
          // B followed by β
          //--------------------------------
          else {
            SymbolID beta = prod.rhs[i + 1];

            if (symbols[beta].type == SymbolType::Terminal) {
              if (follow[B].insert(beta).second)
                changed = true;
            } else {
              for (auto x : first.at(beta)) {
                if (follow[B].insert(x).second)
                  changed = true;
              }
            }
          }
        }
      }
    }

    return follow;
  }

  //----------------------------------------
  // Printing
  //----------------------------------------

  void PrintSet(const std::map<SymbolID, std::set<SymbolID>> &sets,
                const std::string &title) const {
    std::cout << title << "\n";

    for (const auto &symbol : symbols) {
      if (symbol.type != SymbolType::NonTerminal)
        continue;

      std::cout << symbol.name << " : { ";

      auto it = sets.find(symbol.id);

      if (it != sets.end()) {
        for (auto id : it->second)
          std::cout << symbols[id].name << " ";
      }

      std::cout << "}\n";
    }
  }
};
