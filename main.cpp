#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <map>

using namespace std;

enum Space { EMPTY, BLACK, WHITE };
const int SIZE = 8;
const string SAVE_FILE = "player_data.txt";

struct Config {
    int mode; // 1: 1P, 2: 2P
    int cpu_level; // 1: Easy, 2: Normal, 3: Hard
    string black_color = "\033[32m"; // Default Green (for board)
    string stone_b = "B";
    string stone_w = "W";
};

class Othello {
    Space board[SIZE][SIZE];
    map<string, int> player_habits; // 座標ごとの統計

public:
    Othello() {
        for(int i=0; i<SIZE; ++i) for(int j=0; j<SIZE; ++j) board[i][j] = EMPTY;
        board[3][3] = WHITE; board[4][4] = WHITE;
        board[3][4] = BLACK; board[4][3] = BLACK;
        load_learning_data();
    }

    void load_learning_data() {
        ifstream ifs(SAVE_FILE);
        string pos; int count;
        while(ifs >> pos >> count) player_habits[pos] = count;
    }

    void save_learning_data(int r, int c) {
        string pos = to_string(r) + to_string(c);
        player_habits[pos]++;
        ofstream ofs(SAVE_FILE);
        for(auto const& [key, val] : player_habits) ofs << key << " " << val << endl;
    }

    void display(Config& conf) {
        cout << "\n  0 1 2 3 4 5 6 7" << endl;
        for(int i=0; i<SIZE; ++i) {
            cout << i << " ";
            for(int j=0; j<SIZE; ++j) {
                if(board[i][j] == EMPTY) cout << ". ";
                else if(board[i][j] == BLACK) cout << conf.stone_b << " ";
                else cout << conf.stone_w << " ";
            }
            cout << endl;
        }
    }

    bool is_valid(int r, int c, Space s) {
        if(board[r][c] != EMPTY) return false;
        for(int dr=-1; dr<=1; ++dr) {
            for(int dc=-1; dc<=1; ++dc) {
                if(dr==0 && dc==0) continue;
                int nr = r + dr, nc = c + dc;
                bool found_opp = false;
                while(nr>=0 && nr<SIZE && nc>=0 && nc<SIZE && board[nr][nc] == (s == BLACK ? WHITE : BLACK)) {
                    nr += dr; nc += dc; found_opp = true;
                }
                if(found_opp && nr>=0 && nr<SIZE && nc>=0 && nc<SIZE && board[nr][nc] == s) return true;
            }
        }
        return false;
    }

    void place(int r, int c, Space s) {
        board[r][c] = s;
        for(int dr=-1; dr<=1; ++dr) {
            for(int dc=-1; dc<=1; ++dc) {
                if(dr==0 && dc==0) continue;
                int nr = r + dr, nc = c + dc;
                vector<pair<int,int>> to_flip;
                while(nr>=0 && nr<SIZE && nc>=0 && nc<SIZE && board[nr][nc] == (s == BLACK ? WHITE : BLACK)) {
                    to_flip.push_back({nr, nc});
                    nr += dr; nc += dc;
                }
                if(nr>=0 && nr<SIZE && nc>=0 && nc<SIZE && board[nr][nc] == s) {
                    for(auto p : to_flip) board[p.first][p.second] = s;
                }
            }
        }
    }

    pair<int,int> get_cpu_move(int level, Space s) {
        vector<pair<int,int>> moves;
        for(int i=0; i<SIZE; ++i) for(int j=0; j<SIZE; ++j) if(is_valid(i,j,s)) moves.push_back({i,j});
        
        if(level == 1) return moves[rand() % moves.size()];

        // Normal/Hard: 角を狙う or プレイヤーの癖(頻出座標)をブロック
        sort(moves.begin(), moves.end(), [&](pair<int,int> a, pair<int,int> b){
            int score_a = 0, score_b = 0;
            auto eval = [&](int r, int c) {
                int res = 0;
                if((r==0||r==7) && (c==0||c==7)) res += 100; // 角
                string pos = to_string(r) + to_string(c);
                if(level == 3) res += player_habits[pos] * 10; // 学習効果: 相手が好む場所を優先奪取
                return res;
            };
            return eval(a.first, a.second) > eval(b.first, b.second);
        });
        return moves[0];
    }

    bool can_move(Space s) {
        for(int i=0; i<SIZE; ++i) for(int j=0; j<SIZE; ++j) if(is_valid(i,j,s)) return true;
        return false;
    }
};

int main() {
    Config conf;
    cout << "=== OTHELLO C++ EDITION ===" << endl;
    cout << "1: 1 Player (vs CPU)\n2: 2 Players\nSelect: "; cin >> conf.mode;
    if(conf.mode == 1) {
        cout << "CPU Level (1:Easy, 2:Normal, 3:Hard): "; cin >> conf.cpu_level;
    }
    cout << "Enter Black Stone Char (e.g. B): "; cin >> conf.stone_b;
    cout << "Enter White Stone Char (e.g. W): "; cin >> conf.stone_w;

    Othello game;
    Space current = BLACK;
    while(game.can_move(BLACK) || game.can_move(WHITE)) {
        game.display(conf);
        if(!game.can_move(current)) {
            cout << "No moves! Skip." << endl;
            current = (current == BLACK ? WHITE : BLACK);
            continue;
        }

        int r, c;
        if(conf.mode == 1 && current == WHITE) {
            pair<int,int> move = game.get_cpu_move(conf.cpu_level, WHITE);
            r = move.first; c = move.second;
            cout << "CPU placed at " << r << " " << c << endl;
        } else {
            cout << (current == BLACK ? "Black" : "White") << " turn (row col): ";
            cin >> r >> c;
            if(!game.is_valid(r,c,current)) { cout << "Invalid!" << endl; continue; }
            if(current == BLACK) game.save_learning_data(r, c); // プレイヤーの癖を学習
        }
        game.place(r, c, current);
        current = (current == BLACK ? WHITE : BLACK);
    }
    game.display(conf);
    cout << "Game Over!" << endl;
    return 0;
}
