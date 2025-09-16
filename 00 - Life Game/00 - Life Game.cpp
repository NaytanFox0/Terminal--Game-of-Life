// 00 - Life Game.cpp : Este arquivo contém a função 'main'. A execução do programa começa e termina ali.
//
// Executar programa: Ctrl + F5 ou Menu Depurar > Iniciar Sem Depuração
// Depurar programa: F5 ou menu Depurar > Iniciar Depuração
//

#include <iostream>
#include <unordered_set>
#include <vector>
#include <string>
#include <iomanip>
#include <thread>
#include <chrono>
#include <conio.h>
#include <windows.h>

#define Width 84
#define Height 42

bool keys[256]{ true };
int mouseX = 0, mouseY = 0;

// Executa Verificação de clicks e pressed
void inputThread() {
    while (true) {
        for (int i = 0; i < 256; i++) keys[i] = GetAsyncKeyState(i) & 0x8000;
        std::this_thread::yield();
    }
}

// Thread para mouse
void mouseThread() {
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD prevMode;
    GetConsoleMode(hInput, &prevMode);
    SetConsoleMode(hInput, ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT);

    INPUT_RECORD input;
    DWORD events;

    while (true) {
        ReadConsoleInput(hInput, &input, 1, &events);
        if (input.EventType == MOUSE_EVENT) {
            mouseX = input.Event.MouseEvent.dwMousePosition.X / 2;
            mouseY = input.Event.MouseEvent.dwMousePosition.Y - 1;
        }
        std::this_thread::yield();
    }
}

// Modifica um pixel no display 
void setPixel(int x, int y, int color, std::vector<std::vector<std::string>>& display) {
    if(x >= 0 && y >= 0 && x < Width && y < Height) display[y][x] = "\033[48;5;"+std::to_string(color)+"m";
}

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

struct CellHash {
    std::size_t operator()(const Cell& c) const noexcept {
        return std::hash<int>()(c.x) ^ (std::hash<int>()(c.y) << 1);
    }
};

using Board = std::unordered_set<Cell, CellHash>;

// Gera coordenadas dos visinhos
std::vector<Cell> neighbors(const Cell& c) {
    std::vector<Cell> neigh;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            neigh.emplace_back(c.x + dx, c.y + dy, false);
        }
    }
    return neigh;
}

// Conta vizinhos vivos
int countAliveNeigbors(const Board& board, const Cell& c) {
    int count = 0;
    for (auto& n : neighbors(c)) {
        if (board.find(n) != board.end()) count++;
    }
    return count;
}

Board nextGeneration(const Board& current) {
    Board next;

    std::unordered_set<Cell, CellHash> toCheck;
    // Verifica vizinhos vivos
    for (auto& c : current) {
        toCheck.insert(c);
        for (auto& n : neighbors(c)) toCheck.insert(n);
    }

    // Aplicar regras
    for (auto& c : toCheck) {
        bool aliveNow = current.find(c) != current.end();
        int aliveNeighbors = countAliveNeigbors(current, c);

        if (aliveNow) {
            if (c.immortal || aliveNeighbors == 2 || aliveNeighbors == 3)
                next.insert(Cell(c.x, c.y, true)); // novo Cell tem immortal=false por padrão
        }
        else {
            if (aliveNeighbors == 3)
                next.insert(Cell(c.x, c.y, true));
        }
    }

    return next;
}

int main()
{
    // Init
    // Display
    std::vector<std::vector<std::string>> display(Height, std::vector<std::string>(Width, "\033[0m"));
    
    // Maximiza janela e prende cursor
    HWND console = GetConsoleWindow();
    ShowWindow(console, SW_MAXIMIZE);
    SetCursorPos(GetSystemMetrics(SM_CXSCREEN)/2, GetSystemMetrics(SM_CYSCREEN)/2);

	// Habilita ANSI no Windows
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

    // threads
    std::thread tInput(inputThread);
    std::thread tMouse(mouseThread);

    // Clock
    using clock = std::chrono::steady_clock;
    const auto frameTime = std::chrono::milliseconds(32); // FPS

    // Incializa Tabuleiro infinito
    Board board;

    // Adicionar algumas células iniciais
    board.insert(Cell(10, 10));
    board.insert(Cell(11, 10));
    board.insert(Cell(12, 10));

    bool pause = false;

    int x =0, y =0;

    // Gameloop
    while (true) {
        auto start = clock::now();

        // Process Events
        if (keys[VK_ESCAPE]) {   // VK_ESCAPE é a tecla ESC
            std::cout << "\033[2J\033[0;0H"; // limpar tela
            break;               // sai do loop
        }

        if (keys[VK_LBUTTON]) {
            Cell c(mouseX - x, mouseY - y);
            auto it = board.find(c);
            if (it == board.end()) { // nova célula é imortal no próximo tick
                c.immortal = true;
                board.insert(c);
            }
        }

        if (keys[VK_RBUTTON]) {
            Cell c(mouseX - x, mouseY - y);
            auto it = board.find(c);
            if (it != board.end()) board.erase(it); // mata célula existente
        }

        if (keys[VK_SPACE]) {
            if (pause == true) pause = !pause;
            else pause = !pause;
        }

        if (keys['Q']) board.clear();
        if (keys['W']) y++;
        if (keys['A']) x++;
        if (keys['S']) y--;
        if (keys['D']) x--;

        // Render Update
        for (auto& c : board) setPixel(c.x + x, c.y + y, 255, display);
        
        // Render
        std::string frame;
        int i = 0, j = 0;
        for (auto& col : display) {
            for (auto& ch : col) {
                frame += ch + ((mouseY == j && mouseX == i) ? "[]" : "  ") + "\033[0m"; 
                i++;
            }
            frame += "\n";
            j++;
            i = 0;
        }
        std::cout << "\033[?25l\033[0;0H" 
            << "Cursor: " << std::setfill(' ') << std::setw(2) << mouseX << "," << std::setfill(' ') << std::setw(2) << mouseY 
            << " Camera: " << std::setfill(' ') << std::setw(3) << x << "," << std::setfill(' ') << std::setw(3) << y 
            << " Cell: amount - " << std::setfill(' ') << std::setw(3) << board.size() << "\n"
            << frame;
        display.assign(Height, std::vector<std::string>(Width, "\033[0m")); // Restart Display

        // Update
        if (!pause) board = nextGeneration(board);
        
        // Render Delay
        auto elapsed = clock::now() - start;
        if (elapsed < frameTime) std::this_thread::sleep_for(frameTime - elapsed);
    }

    // Detach threads
    tInput.detach();
    tMouse.detach();
    return 0;
}