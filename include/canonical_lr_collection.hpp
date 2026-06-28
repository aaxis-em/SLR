
#pragma once
#include "grammar.hpp"
#include <map>
#include <set>
#include <vector>

struct Item {
  ProductionID prod; // which production
  int dot;           // dot position in RHS (0 … |Rhs|)

  bool operator<(const Item &o) const {
    if (prod != o.prod)
      return prod < o.prod;
    return dot < o.dot;
  }
  bool operator==(const Item &o) const {
    return prod == o.prod && dot == o.dot;
  }
};

using ItemSet = std::set<Item>;

class CanonicalLRCollection {
public:
  std::vector<ItemSet> states;                       // Iₙ
  std::map<std::pair<int, SymbolID>, int> gotoTable; // GOTO(Iᵢ, X) = j

  explicit CanonicalLRCollection(const Grammar &g) {
    const auto &prods = g.GetProductions();
    const auto &symbols = g.GetSymbols();

    // -----------------------------------------------------------------------
    // Helper: closure of an item set
    // -----------------------------------------------------------------------
    auto closure = [&](ItemSet items) -> ItemSet {
      bool changed = true;
      while (changed) {
        changed = false;
        for (const auto &item : items) {
          const auto &prod = prods[item.prod];
          if (item.dot >= (int)prod.rhs.size())
            continue;

          SymbolID B = prod.rhs[item.dot];
          if (symbols[B].type != SymbolType::NonTerminal)
            continue;

          // Add [B -> . γ] for every B-production
          for (const auto &p : prods) {
            if (p.lhs == B) {
              Item ni{p.id, 0};
              if (items.insert(ni).second)
                changed = true;
            }
          }
        }
      }
      return items;
    };

    auto gotoFn = [&](const ItemSet &I, SymbolID X) -> ItemSet {
      ItemSet moved;
      for (const auto &item : I) {
        const auto &prod = prods[item.prod];
        if (item.dot < (int)prod.rhs.size() && prod.rhs[item.dot] == X)
          moved.insert({item.prod, item.dot + 1});
      }
      if (moved.empty())
        return {};
      return closure(moved);
    };

    ItemSet I0 = closure({{0, 0}});
    states.push_back(I0);

    for (int i = 0; i < (int)states.size(); i++) {
      // Try every grammar symbol as the X in GOTO(Iᵢ, X)
      for (const auto &sym : symbols) {
        ItemSet J = gotoFn(states[i], sym.id);
        if (J.empty())
          continue;

        // Check whether J is already in states
        int j = -1;
        for (int k = 0; k < (int)states.size(); k++) {
          if (states[k] == J) {
            j = k;
            break;
          }
        }
        if (j == -1) {
          j = (int)states.size();
          states.push_back(J);
        }
        gotoTable[{i, sym.id}] = j;
      }
    }
  }

  // Pretty-print every state
  void Print(const Grammar &g) const {
    const auto &prods = g.GetProductions();
    const auto &symbols = g.GetSymbols();

    auto symName = [&](SymbolID id) -> std::string {
      for (const auto &s : symbols)
        if (s.id == id)
          return s.name;
      return "?";
    };

    for (int i = 0; i < (int)states.size(); i++) {
      std::cout << "I" << i << ":\n";
      for (const auto &item : states[i]) {
        const auto &prod = prods[item.prod];
        std::cout << "  [" << symName(prod.lhs) << " -> ";
        for (int k = 0; k < (int)prod.rhs.size(); k++) {
          if (k == item.dot)
            std::cout << ". ";
          std::cout << symName(prod.rhs[k]) << " ";
        }
        if (item.dot == (int)prod.rhs.size())
          std::cout << ".";
        std::cout << "]\n";
      }
    }

    std::cout << "\nGOTO transitions:\n";
    for (const auto &[key, dest] : gotoTable) {
      std::cout << "  GOTO(I" << key.first << ", " << symName(key.second)
                << ") = I" << dest << "\n";
    }
  }
};
