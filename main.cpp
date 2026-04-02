#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <algorithm>

const int CELL_SIZE = 64;
const int BOARD_MARGIN = 40;
const int BOARD_SIZE = 8;
const wchar_t* DATA_PATH = L"brain.dat";

enum class Space { EMPTY, BLACK, WHITE };
enum class Scene { TITLE, GAME };

// グローバル状態
Space board[BOARD_SIZE][BOARD_SIZE];
Space current_turn = Space::BLACK;
Scene current_scene = Scene::TITLE;
int cpu_level = 2;
bool vs_cpu = true;
std::map<int, int> player_habits;

// プロトタイプ宣言
void ResetGame();
void LoadHabit();
void SaveHabit(int r, int c);

// --- ロジック ---
void ResetGame() {
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++) board[i][j] = Space::EMPTY;
    board[3][3] = Space::WHITE; board[4][4] = Space::WHITE;
    board[3][4] = Space::BLACK; board[4][3] = Space::BLACK;
    current_turn = Space::BLACK;
}

void LoadHabit() {
    std::ifstream ifs(DATA_PATH, std::ios::binary);
    int pos, count;
    while (ifs.read((char*)&pos, sizeof(pos)) && ifs.read((char*)&count, sizeof(count))) {
        player_habits[pos] = count;
    }
}

void SaveHabit(int r, int c) {
    player_habits[r * 10 + c]++;
    std::ofstream ofs(DATA_PATH, std::ios::binary);
    for (auto const& [pos, count] : player_habits) {
        ofs.write((char*)&pos, sizeof(pos));
        ofs.write((char*)&count, sizeof(count));
    }
}

bool IsValid(int r, int c, Space s) {
    if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || board[r][c] != Space::EMPTY) return false;
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
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
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
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

// --- 描画エンジン ---
void DrawGame(HDC hdc, HWND hwnd) {
    // 背景塗りつぶし
    HBRUSH hBack = CreateSolidBrush(RGB(20, 20, 20));
    RECT client; GetClientRect(hwnd, &client);
    FillRect(hdc, &client, hBack);
    DeleteObject(hBack);

    if (current_scene == Scene::TITLE) {
        // 本気のタイトル画面
        SetBkMode(hdc, TRANSPARENT);
        HFONT hTitleFont = CreateFont(72, 0, 0, 0, FW_EXTRABOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Impact");
        SelectObject(hdc, hTitleFont);
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOut(hdc, 80, 80, L"OTHELLO ELITE", 13);
        DeleteObject(hTitleFont);

        HFONT hMenuFont = CreateFont(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"MS UI Gothic");
        SelectObject(hdc, hMenuFont);
        SetTextColor(hdc, RGB(200, 200, 200));
        TextOut(hdc, 120, 220, L"[1] CPU EASY", 12);
        TextOut(hdc, 120, 260, L"[2] CPU NORMAL", 14);
        TextOut(hdc, 120, 300, L"[3] CPU HARD", 12);
        TextOut(hdc, 120, 340, L"[P] 2-PLAYER MODE", 17);
        DeleteObject(hMenuFont);
    } else {
        // ゲーム画面
        for (int r = 0; r < BOARD_SIZE; r++) {
            for (int c = 0; c < BOARD_SIZE; c++) {
                int x = BOARD_MARGIN + c * CELL_SIZE;
                int y = BOARD_MARGIN + r * CELL_SIZE;
                
                // ガイド（黄土色） or 通常（深緑）
                COLORREF color = IsValid(r, c, current_turn) ? RGB(218, 165, 32) : RGB(34, 139, 34);
                HBRUSH hBr = CreateSolidBrush(color);
                RECT rct = { x, y, x + CELL_SIZE, y + CELL_SIZE };
                FillRect(hdc, &rct, hBr);
                FrameRect(hdc, &rct, (HBRUSH)GetStockObject(BLACK_BRUSH));
                DeleteObject(hBr);

                if (board[r][c] != Space::EMPTY) {
                    HBRUSH sBr = CreateSolidBrush(board[r][c] == Space::BLACK ? RGB(0, 0, 0) : RGB(255, 255, 255));
                    SelectObject(hdc, sBr);
                    Ellipse(hdc, x + 6, y + 6, x + CELL_SIZE - 6, y + CELL_SIZE - 6);
                    DeleteObject(sBr);
                }
            }
        }
        SetTextColor(hdc, RGB(255, 255, 255));
        std::wstring status = L"TURN: " + std::wstring(current_turn == Space::BLACK ? L"BLACK" : L"WHITE");
        TextOut(hdc, BOARD_MARGIN, 10, status.c_str(), (int)status.length());
    }
}

// --- ウィンドウ制御 ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: LoadHabit(); return 0;
    case WM_CHAR:
        if (current_scene == Scene::TITLE) {
            if (wParam == '1' || wParam == '2' || wParam == '3') {
                cpu_level = (int)(wParam - '0'); vs_cpu = true; current_scene = Scene::GAME;
            } else if (wParam == 'p' || wParam == 'P') {
                vs_cpu = false; current_scene = Scene::GAME;
            }
            if (current_scene == Scene::GAME) ResetGame();
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    case WM_LBUTTONDOWN:
        if (current_scene == Scene::GAME) {
            int c = (LOWORD(lParam) - BOARD_MARGIN) / CELL_SIZE;
            int r = (HIWORD(lParam) - BOARD_MARGIN) / CELL_SIZE;
            if (IsValid(r, c, current_turn)) {
                Flip(r, c, current_turn);
                if (current_turn == Space::BLACK) SaveHabit(r, c);
                current_turn = (current_turn == Space::BLACK ? Space::WHITE : Space::BLACK);
                InvalidateRect(hwnd, NULL, TRUE);
                if (vs_cpu && current_turn == Space::WHITE) PostMessage(hwnd, WM_USER + 1, 0, 0);
            }
        }
        return 0;
    case WM_USER + 1: // CPU思考
        Sleep(400);
        {
            std::vector<std::pair<int, int>> moves;
            for(int r=0; r<BOARD_SIZE; r++) for(int c=0; c<BOARD_SIZE; c++)
                if(IsValid(r, c, Space::WHITE)) moves.push_back({r, c});
            if (!moves.empty()) {
                std::sort(moves.begin(), moves.end(), [](auto a, auto b){ return (rand() % 2); });
                Flip(moves[0].first, moves[0].second, Space::WHITE);
            }
            current_turn = Space::BLACK;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawGame(hdc, hwnd);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hI, HINSTANCE, PWSTR, int nS) {
    const wchar_t CN[] = L"OthelloEliteFix";
    WNDCLASS wc = {0}; wc.lpfnWndProc = WindowProc; wc.hInstance = hI; wc.lpszClassName = CN;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(0, CN, L"Othello Elite v2.0", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 600, 680, NULL, NULL, hI, NULL);
    ShowWindow(hwnd, nS);
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}
