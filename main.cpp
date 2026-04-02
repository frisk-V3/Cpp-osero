#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <vector>
#include <string>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// --- 定数・グローバル ---
const int BOARD_SIZE = 8;
const float CELL_SIZE = 64.0f;
const float MARGIN = 50.0f;

enum class Space { EMPTY, BLACK, WHITE };
enum class Scene { TITLE, GAME };

Space board[BOARD_SIZE][BOARD_SIZE];
Scene current_scene = Scene::TITLE;
Space current_turn = Space::BLACK;

// Direct2D / DirectWrite インターフェース
ID2D1Factory* pFactory = NULL;
ID2D1HwndRenderTarget* pRT = NULL;
ID2D1SolidColorBrush* pBrush = NULL;
IDWriteFactory* pDWFactory = NULL;
IDWriteTextFormat* pFormatTitle = NULL;
IDWriteTextFormat* pFormatMenu = NULL;

// --- 初期化と解放 ---
void ResetGame() {
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++) board[i][j] = Space::EMPTY;
    board[3][3] = Space::WHITE; board[4][4] = Space::WHITE;
    board[3][4] = Space::BLACK; board[4][3] = Space::BLACK;
    current_turn = Space::BLACK;
}

HRESULT InitD2D(HWND hwnd) {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
    if (SUCCEEDED(hr)) {
        RECT rc; GetClientRect(hwnd, &rc);
        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(rc.right, rc.bottom)),
            &pRT);
    }
    if (SUCCEEDED(hr)) {
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWFactory));
        pDWFactory->CreateTextFormat(L"Impact", NULL, DWRITE_FONT_WEIGHT_HEAVY, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 72.0f, L"ja-JP", &pFormatTitle);
        pDWFactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 24.0f, L"ja-JP", &pFormatMenu);
    }
    return hr;
}

void CleanD2D() {
    if (pFormatMenu) pFormatMenu->Release();
    if (pFormatTitle) pFormatTitle->Release();
    if (pDWFactory) pDWFactory->Release();
    if (pBrush) pBrush->Release();
    if (pRT) pRT->Release();
    if (pFactory) pFactory->Release();
}

// --- 描画エンジン ---
void Render(HWND hwnd) {
    if (!pRT) return;
    pRT->BeginDraw();
    pRT->Clear(D2D1::ColorF(0.08f, 0.08f, 0.1f)); // 深みのあるダークネイビー

    if (current_scene == Scene::TITLE) {
        pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBrush);
        pRT->DrawTextW(L"OTHELLO ELITE", 13, pFormatTitle, D2D1::RectF(50, 100, 600, 200), pBrush);
        pRT->DrawTextW(L"PRESS ANY KEY TO START", 23, pFormatMenu, D2D1::RectF(80, 300, 600, 400), pBrush);
        pBrush->Release();
    } else {
        for (int r = 0; r < BOARD_SIZE; r++) {
            for (int c = 0; c < BOARD_SIZE; c++) {
                D2D1_RECT_F rect = D2D1::RectF(MARGIN + c * CELL_SIZE, MARGIN + r * CELL_SIZE, MARGIN + (c + 1) * CELL_SIZE, MARGIN + (r + 1) * CELL_SIZE);
                
                // ガイドは黄土色（Goldenrod）
                bool can_place = false; // 本来はここにロジックを入れる
                pRT->CreateSolidColorBrush(can_place ? D2D1::ColorF(0.8f, 0.6f, 0.1f) : D2D1::ColorF(0.12f, 0.4f, 0.23f), &pBrush);
                pRT->FillRectangle(rect, pBrush);
                pBrush->Release();

                pRT->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.5f), &pBrush);
                pRT->DrawRectangle(rect, pBrush, 1.0f);
                pBrush->Release();

                if (board[r][c] != Space::EMPTY) {
                    D2D1_ELLIPSE ell = D2D1::Ellipse(D2D1::Point2F(rect.left + CELL_SIZE/2, rect.top + CELL_SIZE/2), CELL_SIZE/2 - 6, CELL_SIZE/2 - 6);
                    pRT->CreateSolidColorBrush(board[r][c] == Space::BLACK ? D2D1::ColorF(0.05f, 0.05f, 0.05f) : D2D1::ColorF(0.95f, 0.95f, 0.95f), &pBrush);
                    pRT->FillEllipse(ell, pBrush);
                    pBrush->Release();
                }
            }
        }
    }
    pRT->EndDraw();
}

// --- プロシージャ ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: InitD2D(hwnd); ResetGame(); return 0;
    case WM_KEYDOWN: 
        if(current_scene == Scene::TITLE) { current_scene = Scene::GAME; InvalidateRect(hwnd, NULL, FALSE); }
        return 0;
    case WM_PAINT: Render(hwnd); ValidateRect(hwnd, NULL); return 0;
    case WM_SIZE:
        if (pRT) {
            RECT rc; GetClientRect(hwnd, &rc);
            pRT->Resize(D2D1::SizeU(rc.right, rc.bottom));
        }
        return 0;
    case WM_DESTROY: CleanD2D(); PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hI, HINSTANCE, PWSTR, int nS) {
    const wchar_t CLASS_NAME[] = L"EliteD2DClass";
    WNDCLASSW wc = {0}; // 明示的にW版を使用
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hI;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Othello Elite Direct2D", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 620, 680, NULL, NULL, hI, NULL);

    ShowWindow(hwnd, nS);
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}
