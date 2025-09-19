// 00 - Life Game.cpp : Este arquivo contém a função 'main'. A execução do programa começa e termina ali.
//
// Executar programa: Ctrl + F5 ou Menu Depurar > Iniciar Sem Depuração
// Depurar programa: F5 ou menu Depurar > Iniciar Depuração
//

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <thread>
#include <chrono>
#include <windows.h>
#include "Board.h"

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
            mouseY = input.Event.MouseEvent.dwMousePosition.Y;
        }
        std::this_thread::yield();
    }
}

// Mapeia n (0-15) para cor do console
WORD getColor(int n) {
    switch (n) {
    case 0: return 0; // Preto
    case 1: return BACKGROUND_BLUE;
    case 2: return BACKGROUND_GREEN;
    case 3: return BACKGROUND_GREEN | BACKGROUND_BLUE; // Cyan
    case 4: return BACKGROUND_RED;
    case 5: return BACKGROUND_RED | BACKGROUND_BLUE; // Magenta
    case 6: return BACKGROUND_RED | BACKGROUND_GREEN; // Amarelo
    case 7: return BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE; // Branco claro
    case 8: return BACKGROUND_INTENSITY; // cinza escuro (ou ajustar)
    case 9: return BACKGROUND_BLUE | BACKGROUND_INTENSITY;
    case 10: return BACKGROUND_GREEN | BACKGROUND_INTENSITY;
    case 11: return BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
    case 12: return BACKGROUND_RED | BACKGROUND_INTENSITY;
    case 13: return BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
    case 14: return BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
    case 15: return BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY; // Branco total
    case 16: return FOREGROUND_BLUE;
    case 17: return FOREGROUND_GREEN;
    case 18: return FOREGROUND_GREEN | FOREGROUND_BLUE; // Cyan
    case 19: return FOREGROUND_RED;
    case 20: return FOREGROUND_RED | FOREGROUND_BLUE; // Magenta
    case 21: return FOREGROUND_RED | FOREGROUND_GREEN; // Amarelo
    case 22: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // Branco claro
    case 23: return FOREGROUND_INTENSITY; // cinza escuro (ou ajustar)
    case 24: return FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    case 25: return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    case 26: return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    case 27: return FOREGROUND_RED | FOREGROUND_INTENSITY;
    case 28: return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    case 29: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    case 30: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; // Branco total

    default: return 0;
    }
}

// Classe para manipulação do console como uma tela gráfica
class Termat {
private:
    std::vector<CHAR_INFO> buffer;
    COORD bufferSize;
    SMALL_RECT writeRegion;
    HANDLE hConsole;
    const int width;
    const int height;

public:
    Termat(SHORT width, SHORT height, char asciiChar = ' ', WORD attributes = getColor(0)|getColor(30)) : buffer((width * 2) * height), bufferSize{width * 2, height}, writeRegion{0, 0, width * 2 - 1, height - 1}, hConsole(GetStdHandle(STD_OUTPUT_HANDLE)), width(width), height(height) {
        // Inicializa buffer
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                CHAR_INFO ci;
                ci.Char.AsciiChar = asciiChar;
                ci.Attributes = attributes;

                int idx = y * (width * 2) + x * 2; // mapeia pixel virtual para 2 chars reais
                buffer[idx] = ci;
                buffer[idx + 1] = ci;

            }
        }

        // Maximiza janela e prende cursor
        ShowWindow(GetConsoleWindow(), SW_MAXIMIZE);
		SetCursorPos(GetSystemMetrics(SM_CXSCREEN) / 2, GetSystemMetrics(SM_CYSCREEN) / 2);

        // Habilita ANSII no Windows
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);

		// Desabilita redimensionamento da janela
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);

        // Deixa o buffer com o mesmo tamanho da janela
        SMALL_RECT rect = csbi.srWindow;
        COORD newSize;
        newSize.X = rect.Right - rect.Left + 1;
        newSize.Y = rect.Bottom - rect.Top + 1;
        
        SetConsoleScreenBufferSize(hConsole, newSize);

        // Oculta o cursor do console
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = FALSE;   // deixa invisível
        SetConsoleCursorInfo(hConsole, &cursorInfo);
    }

    ~Termat() {};

    void setCh(int x, int y, char ch, WORD attributes) {
        if (x >= 0 && y >= 0 && x < width && y < height) {
            int idx = y * (width * 2) + x * 2;
            CHAR_INFO ci = { ch, attributes };
            buffer[idx] = ci;
            buffer[idx + 1] = ci;
        }
    }

    void setString(int x, int y, const std::string& text, WORD attributes = getColor(0)|getColor(30)) {
        if (y < 0 || y >= height) return; // fora da tela

        for (size_t i = 0; i < text.size(); i++) {
            int px = x + i;
            if (px < 0 || px >= width * 2) break; // garante dentro do buffer real

            int idx = y * (width * 2) + px; // agora é 1x1, sem duplicar
            CHAR_INFO ci;
            ci.Char.AsciiChar = text[i];
            ci.Attributes = attributes;
            buffer[idx] = ci;
        }
    }


    void setBuffer(char asciiChar = ' ', WORD attributes = getColor(0) | getColor(30)) {
        CHAR_INFO ci;
        ci.Char.AsciiChar = asciiChar;
        ci.Attributes = attributes;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = y * (width * 2) + x * 2;
                buffer[idx] = ci;
                buffer[idx + 1] = ci;
            }
        }
    }

    void render() {
        COORD bufferCoord{ 0, 0 };
		WriteConsoleOutputA(hConsole, buffer.data(), bufferSize, bufferCoord, &writeRegion);
    }
};

int main() {
    // Init
    // Display
    Termat termat(88, 44);

    // threads
    std::thread tInput(inputThread);
    std::thread tMouse(mouseThread);

    // Clock
    using clock = std::chrono::steady_clock;
    const auto frameTime = std::chrono::milliseconds(32); // FPS

    // Incializa Tabuleiro infinito
    Board board;

    board.insert(Cell(10, 10));
    board.insert(Cell(11, 10));
    board.insert(Cell(12, 10));

    // outros
    bool pause = false;
    int x = 0, y = 0;

    // Gameloop
    while (true) {
        auto start = clock::now();

        // Process Events
        if (keys[VK_ESCAPE]) {   // VK_ESCAPE é a tecla ESC
            std::cout << "\033[2J\033[H"; // limpar tela
            break;               // sai do loop
        }

        // Criar e Destruir células
        Cell c(mouseX - x, mouseY - y);
        bool exists = board.find(c) != board.end();

        if (keys[VK_LBUTTON] && !exists) {
            c.immortal = true;
            board.insert(c);
        }

        if (keys[VK_RBUTTON] && exists) {
            board.erase(c);
        }

        // Pausar
        if (keys[VK_SPACE]) {
            if (pause == true) pause = !pause;
            else pause = !pause;
        }

        // Movimentar câmera
        if (keys['Q']) board.clear(); // Limpar tabuleiro
        if (keys['W']) y++;
        if (keys['A']) x++;
        if (keys['S']) y--;
        if (keys['D']) x--;

        // Render 
        // Board
        bool mouseOverCell = false;
        for (auto& c : board) {
            termat.setCh(c.x + x, c.y + y, ' ', getColor(15)|getColor(30));
            if (c.x + x == mouseX && c.y + y == mouseY) mouseOverCell = true;
        }

        // Cursor
        termat.setCh(mouseX, mouseY, '|', mouseOverCell ? getColor(15) | getColor(16) : getColor(0) | getColor(30));

        // info
		termat.setString(0, 0, "WASD - Move | LMB - Create | RMB - Destroy | SPACE - Pause | Q - Clear | ESC - Exit");
        char buf[128];
        sprintf_s(buf, "Cursor: %2d,%2d Camera: %3d,%3d Cell: amount - %4zu", mouseX, mouseY, x, y, board.size());
        termat.setString(0, 1, buf);

        termat.render();
		termat.setBuffer(); // limpa buffer

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