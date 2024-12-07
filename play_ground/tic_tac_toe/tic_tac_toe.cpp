#include <graphics.h>

#include <cstdlib>
#include <string>

struct Position {
    int x = 0;
    int y = 0;
};

static constexpr int WIDTH = 600;
static constexpr int HEIGHT = 600;
static constexpr int CELL_WIDTH = 200;
static constexpr char INITIAL = ' ';
static constexpr char PLAYER1 = 'O';
static constexpr char PLAYER2 = 'X';
static char next_player_ = PLAYER1;
static char current_player_ = next_player_;

char board[3][3] {
    INITIAL, INITIAL, INITIAL,
    INITIAL, INITIAL, INITIAL,
    INITIAL, INITIAL, INITIAL
};

static constexpr char next_player() noexcept
{
    current_player_ = next_player_;
    next_player_ = next_player_ == PLAYER1 ? PLAYER2 : PLAYER1;
    return current_player_;
}

static constexpr char current_player() noexcept
{
    return current_player_;
}

static bool is_won(char player) noexcept
{
    if (board[0][0] == player && board[0][1] == player && board[0][2] == player)
        return true;
    if (board[1][0] == player && board[1][1] == player && board[1][2] == player)
        return true;
    if (board[2][0] == player && board[2][1] == player && board[2][2] == player)
        return true;
    if (board[0][0] == player && board[1][0] == player && board[2][0] == player)
        return true;
    if (board[0][1] == player && board[1][1] == player && board[2][1] == player)
        return true;
    if (board[0][2] == player && board[1][2] == player && board[2][2] == player)
        return true;
    if (board[0][0] == player && board[1][1] == player && board[2][2] == player)
        return true;
    if (board[0][2] == player && board[1][1] == player && board[2][0] == player)
        return true;
    return false;
}

static bool is_draw() noexcept
{
    for (const auto& piece: board) {
        for (const auto& c: piece) {
            if (c == INITIAL) return false;
        }
    }

    return true;
}

static void draw_board() noexcept
{
    ::line(0, 200, 600, 200);
    ::line(0, 400, 600, 400);
    ::line(200, 0, 200, 600);
    ::line(400, 0, 400, 600);
}

static void draw_chess() noexcept
{
    int x= 0;
    int y = 0;
    constexpr int radius = CELL_WIDTH / 2;
    for (const auto& piece: board) {
        for (const auto& cell: piece) {
            if (cell == PLAYER1) {
                ::circle(x + radius, y + radius, radius);
            }
            else if (cell == PLAYER2) {
                ::line(x, y, x + CELL_WIDTH, y + CELL_WIDTH);
                ::line(x + CELL_WIDTH, y, x, y + CELL_WIDTH);
            }

            x += CELL_WIDTH;
        }

        x = 0;
        y += CELL_WIDTH;
    }
}

static void draw_tip_text()
{
    TCHAR buffer[64];
    _stprintf_s(buffer, 64, _T("current player is %c"), current_player());

    settextcolor(RGB(224, 175, 45));
    outtextxy(0, 0, buffer);
}

int main(int argc, char* argv[])
{
    initgraph(WIDTH, HEIGHT);

    ExMessage message{};
    BeginBatchDraw();

    bool should_running = true;
    while (should_running) {
        cleardevice();

        while (peekmessage(&message)) {
            if (message.message == WM_LBUTTONDOWN) {
                const int index_x = message.x / CELL_WIDTH;
                const int index_y = message.y / CELL_WIDTH;

                if (board[index_y][index_x] == INITIAL)
                    board[index_y][index_x] = next_player();
            }
        }

        draw_board();
        draw_chess();
        draw_tip_text();
        FlushBatchDraw();

        if (is_won(PLAYER1)) {
            MessageBox(GetHWnd(), _T("player1 won"), _T("game over"), MB_OK);
            should_running = false;
        }
        else if (is_won(PLAYER2)) {
            MessageBox(GetHWnd(), _T("player2 won"), _T("game over"), MB_OK);
            should_running = false;
        }
        else if (is_draw()) {
            MessageBox(GetHWnd(), _T("draw"), _T("game over"), MB_OK);
            should_running = false;
        }

    }

    EndBatchDraw();
    closegraph();
    return EXIT_SUCCESS;
}
