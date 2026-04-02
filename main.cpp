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

// プロトタイプ
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

// --- ダブルバッファリング描画 ---
void Render(HDC hdc, HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // メモリDCの作成
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    SelectObject(memDC, memBitmap);

    // 背景：目に優しいダークグレー
    HBRUSH hBack = CreateSolidBrush(RGB(30, 30, 35));
    FillRect(memDC, &rect, hBack);
    DeleteObject(hBack);

    if (current_scene == Scene::TITLE) {
        SetBkMode(memDC, TRANSPARENT);
        HFONT hTitleFont = CreateFont(80, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Verdana");
        SelectObject(memDC, hTitleFont);
        SetTextColor(memDC, RGB(255, 255, 255));
        TextOut(memDC, 50, 100, L"OTHELLO", 7);
        TextOut(memDC, 80, 180, L"ELITE", 5);
        DeleteObject(hTitleFont);

        HFONT hSubFont = CreateFont(22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Consolas");
        SelectObject(memDC, hSubFont);
        SetTextColor(memDC, RGB(180, 180, 180));
        TextOut(memDC, 60, 320, L"PRESS [1-3] FOR CPU  |  PRESS [P] FOR 2P", 40);
        DeleteObject(hSubFont);
    } else {
        // ボード描画
        for (int r = 0; r < BOARD_SIZE; r++) {
            for (int c = 0; c < BOARD_SIZE; c++) {
                int x = BOARD_MARGIN + c * CELL_SIZE;
                int y = BOARD_MARGIN + r * CELL_SIZE;
                
                // ガイド：黄土色 (RGB 200, 160, 50)
                bool can_place = IsValid(r, c, current_turn);
                COLORREF tileColor = can_place ? RGB(200, 160, 50) : RGB(40, 100, 60);
                HBRUSH hBr = CreateSolidBrush(tileColor);
                RECT rct = { x, y, x + CELL_SIZE, y + CELL_SIZE };
                FillRect(memDC, &rct, hBr);
                FrameRect(memDC, &rct, (HBRUSH)GetStockObject(BLACK_BRUSH));
                DeleteObject(hBr);

                if (board[r][c] != Space::EMPTY) {
                    HBRUSH sBr = CreateSolidBrush(board[r][c] == Space::BLACK ? RGB(10, 10, 10) : RGB(245, 245, 245));
                    SelectObject(memDC, sBr);
                    Ellipse(memDC, x + 8, y + 8, x + CELL_SIZE - 8, y + CELL_SIZE - 8);
                    DeleteObject(sBr);
                }
            }
        }
        SetTextColor(memDC, RGB(255, 255, 255));
        std::wstring status = L"TURN: " + std::wstring(current_turn == Space::BLACK ? L"BLACK" : L"WHITE");
        TextOut(memDC, BOARD_MARGIN, 10, status.c_str(), (int)status.length());
    }

    // 画面へ一気に転送
    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    // 解放
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: LoadHabit(); return 0;
    case WM_ERASEBKGND: return 1; // 背景消去を無視してフラッシュを防ぐ
    case WM_CHAR:
        if (current_scene == Scene::TITLE) {
            if (wParam >= '1' && wParam <= '3') {
                cpu_level = (int)(wParam - '0'); vs_cpu = true; current_scene = Scene::GAME; ResetGame();
            } else if (wParam == 'p' || wParam == 'P') {
                vs_cpu = false; current_scene = Scene::GAME; ResetGame();
            }
            InvalidateRect(hwnd, NULL, FALSE);
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
                InvalidateRect(hwnd, NULL, FALSE);
                if (vs_cpu && current_turn == Space::WHITE) PostMessage(hwnd, WM_USER + 1, 0, 0);
            }
        }
        return 0;
    case WM_USER + 1:
        Sleep(300);
        {
            std::vector<std::pair<int, int>> moves;
            for(int r=0; r<BOARD_SIZE; r++) for(int c=0; c<BOARD_SIZE; c++)
                if(IsValid(r, c, Space::WHITE)) moves.push_back({r, c});
            if (!moves.empty()) {
                std::sort(moves.begin(), moves.end(), [](auto a, auto b){ return (rand() % 2); });
                Flip(moves[0].first, moves[0].second, Space::WHITE);
            }
            current_turn = Space::BLACK;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        Render(hdc, hwnd);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hI, HINSTANCE, PWSTR, int nS) {
    const wchar_t CN[] = L"OthelloElitePro";
    WNDCLASS wc = {0}; wc.lpfnWndProc = WindowProc; wc.hInstance = hI; wc.lpszClassName = CN;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(0, CN, L"OTHELLO ELITE PRO", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 600, 650, NULL, NULL, hI, NULL);
    ShowWindow(hwnd, nS);
    UpdateWindow(hwnd); // 起動直後に描画を強制
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}
