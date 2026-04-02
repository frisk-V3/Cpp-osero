#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <algorithm>

// --- 定数定義 (名前衝突回避のためBOARD_SIZEに変更) ---
const int CELL_SIZE = 60;
const int BOARD_MARGIN = 50;
const int BOARD_SIZE = 8; 
const wchar_t* DATA_PATH = L"brain.dat";

enum class Space { EMPTY, BLACK, WHITE };

// --- グローバル変数 ---
Space board[BOARD_SIZE][BOARD_SIZE];
Space current_turn = Space::BLACK;
bool vs_cpu = true;
std::map<int, int> player_habits;

// --- 学習・ロジック関数 ---
void SaveHabit(int r, int c) {
    player_habits[r * 10 + c]++;
    std::ofstream ofs(DATA_PATH, std::ios::binary);
    for (auto const& [pos, count] : player_habits) {
        ofs.write((char*)&pos, sizeof(pos));
        ofs.write((char*)&count, sizeof(count));
    }
}

void LoadHabit() {
    std::ifstream ifs(DATA_PATH, std::ios::binary);
    int pos, count;
    while (ifs.read((char*)&pos, sizeof(pos)) && ifs.read((char*)&count, sizeof(count))) {
        player_habits[pos] = count;
    }
}

bool IsValid(int r, int c, Space s) {
    if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || board[r][c] != Space::EMPTY) return false;
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int nr = r + dr, nc = c + dc;
            bool found_opp = false;
            while (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board[nr][nc] == (s == Space::BLACK ? Space::WHITE : Space::BLACK)) {
                nr += dr; nc += dc; found_opp = true;
            }
            if (found_opp && nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board[nr][nc] == s) return true;
        }
    }
    return false;
}

void Flip(int r, int c, Space s) {
    board[r][c] = s;
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int nr = r + dr, nc = c + dc;
            std::vector<std::pair<int, int>> targets;
            while (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board[nr][nc] == (s == Space::BLACK ? Space::WHITE : Space::BLACK)) {
                targets.push_back({nr, nc});
                nr += dr; nc += dc;
            }
            if (!targets.empty() && nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board[nr][nc] == s) {
                for (auto& p : targets) board[p.first][p.second] = s;
            }
        }
    }
}

// --- GUI描画 ---
void DrawBoard(HDC hdc) {
    HBRUSH hBg = CreateSolidBrush(RGB(34, 139, 34));
    SelectObject(hdc, hBg);
    Rectangle(hdc, BOARD_MARGIN, BOARD_MARGIN, BOARD_MARGIN + BOARD_SIZE * CELL_SIZE, BOARD_MARGIN + BOARD_SIZE * CELL_SIZE);
    DeleteObject(hBg);

    for (int i = 0; i <= BOARD_SIZE; ++i) {
        MoveToEx(hdc, BOARD_MARGIN + i * CELL_SIZE, BOARD_MARGIN, NULL);
        LineTo(hdc, BOARD_MARGIN + i * CELL_SIZE, BOARD_MARGIN + BOARD_SIZE * CELL_SIZE);
        MoveToEx(hdc, BOARD_MARGIN, BOARD_MARGIN + i * CELL_SIZE, NULL);
        LineTo(hdc, BOARD_MARGIN + BOARD_SIZE * CELL_SIZE, BOARD_MARGIN + i * CELL_SIZE);
    }

    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if (board[r][c] != Space::EMPTY) {
                HBRUSH sBrush = CreateSolidBrush(board[r][c] == Space::BLACK ? RGB(0, 0, 0) : RGB(255, 255, 255));
                SelectObject(hdc, sBrush);
                Ellipse(hdc, BOARD_MARGIN + c * CELL_SIZE + 5, BOARD_MARGIN + r * CELL_SIZE + 5,
                        BOARD_MARGIN + (c + 1) * CELL_SIZE - 5, BOARD_MARGIN + (r + 1) * CELL_SIZE - 5);
                DeleteObject(sBrush);
            }
        }
    }
}

// --- ウィンドウ処理 ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        board[3][3] = Space::WHITE; board[4][4] = Space::WHITE;
        board[3][4] = Space::BLACK; board[4][3] = Space::BLACK;
        LoadHabit();
        return 0;
    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam), y = HIWORD(lParam);
        int c = (x - BOARD_MARGIN) / CELL_SIZE;
        int r = (y - BOARD_MARGIN) / CELL_SIZE;
        if (IsValid(r, c, current_turn)) {
            Flip(r, c, current_turn);
            if (current_turn == Space::BLACK) SaveHabit(r, c);
            current_turn = Space::WHITE;
            InvalidateRect(hwnd, NULL, TRUE);
            if (vs_cpu) PostMessage(hwnd, WM_USER + 1, 0, 0);
        }
        return 0;
    }
    case WM_USER + 1: { // CPU思考 (簡易AI)
        Sleep(300);
        for(int r=0; r<BOARD_SIZE; ++r) for(int c=0; c<BOARD_SIZE; ++c) {
            if (IsValid(r, c, Space::WHITE)) {
                Flip(r, c, Space::WHITE);
                current_turn = Space::BLACK;
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }
        }
        current_turn = Space::BLACK;
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawBoard(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"OthelloClass";
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Elite Othello GUI", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 650, NULL, NULL, hInstance, NULL);
    if (!hwnd) return 0;
    ShowWindow(hwnd, nCmdShow);
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}
