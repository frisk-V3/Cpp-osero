# OTHELLO ELITE v2.0.0

**OTHELLO ELITE** は、Win32 API と Direct2D を駆使して開発された、超高速・低遅延のデスクトップ・オセロゲームです。従来の GDI 描画を廃止し、GPU 加速による描画エンジンを搭載することで、一切の処理落ちを排除した「プロ仕様」の操作感を実現しました。

## 特徴
- **Direct2D / DirectWrite 採用**: GPU によるハードウェア描画で、高フレームレートかつ滑らかなグラフィックスを提供。
- **ハイブリッド CPU**: 盤面の重み付けアルゴリズム（角優先・危険地帯回避）を搭載し、戦略的な対局が可能。
- **マルチモード**: タイトル画面から「1人用（VS CPU）」と「2人用（ローカル対戦）」を即座に選択可能。
- **インテリジェント・ガイド**: 次に置ける場所をリアルタイムに計算し、黄土色（Goldenrod）でハイライト表示。

## 操作方法
- **タイトル画面**
  - `1` キー: PLAYER vs CPU モード開始
  - `2` キー: PLAYER vs PLAYER モード開始
- **ゲーム画面**
  - マウス左クリック: 石を置く
  - 自動パス機能: 置ける場所がない場合は自動でターンがスキップされます。

## ビルド環境
- **OS**: Windows 10/11
- **Compiler**: MSVC (cl.exe) / C++17 標準
- **Libraries**: `user32.lib`, `gdi32.lib`, `d2d1.lib`, `dwrite.lib`

### コンパイルコマンド
```bash
cl.exe /O2 /MT /EHsc /std:c++17 main.cpp /Fe:Othello_Elite.exe user32.lib gdi32.lib d2d1.lib dwrite.lib /link /SUBSYSTEM:WINDOWS
