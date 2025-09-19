#pragma once

// Bibliotecas
#include <unordered_set>
#include <vector>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <conio.h>
#include <windows.h>

// Célula
class Cell {
public:
    bool isAlive;
    bool immortal = false;
    int x, y;

    Cell(int x, int y, bool isAlive = true) : x(x), y(y), isAlive(isAlive) {}

    bool operator==(const Cell& other) const {
        return x == other.x && y == other.y;
    }
};

// Hash para Cell
struct CellHash {
    std::size_t operator()(const Cell& c) const noexcept {
        return std::hash<int>()(c.x) ^ (std::hash<int>()(c.y) << 1);
    }
};

// Tabuleiro
using Board = std::unordered_set<Cell, CellHash>;

// Gera coordenadas dos visinhos
// 
// Array fixo de offsets para vizinhos
constexpr int dx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
constexpr int dy[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
//
// Função otimizada que aplica uma lambda a cada vizinho
template<typename Func>
void neighbor(const Cell& c, Func f) {
    for (int i = 0; i < 8; i++) {
        f(Cell(c.x + dx[i], c.y + dy[i], false));
    }
}

// Conta vizinhos vivos
#include <unordered_map>

Board nextGeneration(const Board& current) {
    Board next;
    std::unordered_map<Cell, int, CellHash> neighborCount;
    neighborCount.reserve(current.size() * 9); // otimização

    // Contagem de vizinhos
    for (const auto& c : current) {
        neighbor(c, [&](const Cell& n) { neighborCount[n]++; });
    }

    // Aplicação das regras
    for (const auto& pair : neighborCount) {
        const Cell& pos = pair.first;
        bool aliveNow = current.find(pos) != current.end();

        if ((aliveNow && (pos.immortal || pair.second == 2 || pair.second == 3)) || (!aliveNow && pair.second == 3)) {
            Cell temp(pos.x, pos.y, true);
            next.insert(temp);
        }
    }

    return next;
}