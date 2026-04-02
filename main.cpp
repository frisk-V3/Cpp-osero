#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <vector>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// --- 定数 ---
const int BOARD_SIZE = 8;
const float CELL_SIZE = 64.0f;
const float MARGIN = 50.0f;

enum class Space { EMPTY, BLACK, WHITE };
enum class Scene { TITLE, GAME };

// --- グローバル変数 ---
Space board[BOARD_SIZE][BOARD_SIZE];
Scene current_scene = Scene::TITLE;
Space current_turn = Space::BLACK;

ID2D1Factory* pFactory = NULL;
ID2D1HwndRenderTarget* pRT = NULL;
IDWriteFactory* pDWFactory = NULL;
IDWriteTextFormat* pFormatTitle = NULL;
IDWriteTextFormat* pFormatMenu = NULL;

// --- ロジック関数 ---
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

void ResetGame() {
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++) board[i][j] = Space::EMPTY;
    board[3][3] = Space::WHITE; board[4][4] = Space::WHITE;
    board[3][4] = Space::BLACK; board[4][3] = Space::BLACK;
    current_turn = Space::BLACK;
}

// --- 描画 ---
void Render(HWND hwnd) {
    if (!pRT) return;
    pRT->BeginDraw();
    pRT->Clear(D2D1::ColorF(0.1f, 0.1f, 0.12f));

    ID2D1SolidColorBrush* pBrush = NULL;

    if (current_scene == Scene::TITLE) {
        pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBrush);
        pRT->DrawTextW(L"OTHELLO ELITE", 13, pFormatTitle, D2D1::RectF(50, 100, 600, 300), pBrush);
        pRT->DrawTextW(L"CLICK TO START", 14, pFormatMenu, D2D1::RectF(50, 300, 600, 400), pBrush);
    } else {
        for (int r = 0; r < BOARD_SIZE; r++) {
            for (int c = 0; c < BOARD_SIZE; c++) {
                D2D1_RECT_F rect = D2D1::RectF(MARGIN + c * CELL_SIZE, MARGIN + r * CELL_SIZE, MARGIN + (c + 1) * CELL_SIZE, MARGIN + (r + 1) * CELL_SIZE);
                
                // ガイド：置ける場所を黄土色、それ以外を深緑
                bool can = IsValid(r, c, current_turn);
                pRT->CreateSolidColorBrush(can ? D2D1::ColorF(0.8f, 0.6f, 0.2f) : D2D1::ColorF(0.15f, 0.45f, 0.25f), &pBrush);
                pRT->FillRectangle(rect, pBrush);
                pBrush->Release();

                pRT->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), &pBrush);
                pRT->DrawRectangle(rect, pBrush, 1.0f);
                pBrush->Release();

                if (board[r][c] != Space::EMPTY) {
                    D2D1_ELLIPSE ell = D2D1::Ellipse(D2D1::Point2F(rect.left + 32, rect.top + 32), 26, 26);
                    pRT->CreateSolidColorBrush(board[r][c] == Space::BLACK ? D2D1::ColorF(0, 0, 0) : D2D1::ColorF(1, 1, 1), &pBrush);
                    pRT->FillEllipse(ell, pBrush);
                    pBrush->Release();
                }
            }
        }
        pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBrush);
        std::wstring status = L"TURN: " + std::wstring(current_turn == Space::BLACK ? L"BLACK" : L"WHITE");
        pRT->DrawTextW(status.c_str(), (UINT32)status.length(), pFormatMenu, D2D1::RectF(50, 10, 500, 50), pBrush);
    }

    if (pBrush) pBrush->Release();
    pRT->EndDraw();
}

// --- Windows処理 ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
        RECT rc; GetClientRect(hwnd, &rc);
        pFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(rc.right, rc.bottom)), &pRT);
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&pDWFactory);
        pDWFactory->CreateTextFormat(L"Impact", NULL, DWRITE_FONT_WEIGHT_HEAVY, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 72.0f, L"ja-JP", &pFormatTitle);
        pDWFactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 20.0f, L"ja-JP", &pFormatMenu);
        ResetGame();
        return 0;
    }
    case WM_LBUTTONDOWN: {
        if (current_scene == Scene::TITLE) {
            current_scene = Scene::GAME;
        } else {
            int c = (int)((LOWORD(lParam) - MARGIN) / CELL_SIZE);
            int r = (int)((HIWORD(lParam) - MARGIN) / CELL_SIZE);
            if (IsValid(r, c, current_turn)) {
                Flip(r, c, current_turn);
                current_turn = (current_turn == Space::BLACK ? Space::WHITE : Space::BLACK);
            }
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_PAINT: Render(hwnd); ValidateRect(hwnd, NULL); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hI, HINSTANCE, PWSTR, int nS) {
    WNDCLASSW wc = {0}; wc.lpfnWndProc = WindowProc; wc.hInstance = hI; wc.lpszClassName = L"OthelloW";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClassW(&wc);
    HWND hwnd = CreateWindowExW(0, L"OthelloW", L"Direct2D Othello Elite", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 620, 680, NULL, NULL, hI, NULL);
    ShowWindow(hwnd, nS);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}
