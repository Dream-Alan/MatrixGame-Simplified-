//  模块 5：main.cpp —— 程序组装：进度存档、启动器、游戏界面
//
//  这一层只做三件事：把 Game 里的矩阵画成格子、把按键转发给 Game、
//  把通关进度存到文件里。真正的规则在 game.cpp，控件细节在 ui.cpp。
#include <windows.h>

#include <cstdio>
#include <ctime>
#include <memory>
#include <string>
#include <vector>
using namespace std;

#include "game.h"
#include "ui.h"

namespace {
// 进度存档：一个文件里就三个整数（化简 / 复原 / 求逆 各通关多少局）
struct Progress {
    int value[3] = {0, 0, 0};
    int& operator[](Mode m) { return value[int(m)]; }   // 用模式当下标，写起来更直观
};

// 存档放在 exe 旁边。这里用 _wfopen 而不是 ofstream：本工程路径带中文
// （C++课程设计），窄字符 ofstream 会按 ANSI 代码页解释文件名，静默打开失败。
wstring savePath() {
    wchar_t buf[MAX_PATH] = {0};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);      // 取 exe 的完整路径
    wstring path(buf);
    const size_t slash = path.find_last_of(L'\\');   // 找到最后一个反斜杠
    return slash == wstring::npos ? L"progress.txt" : path.substr(0, slash) + L"\\progress.txt";
}

Progress loadProgress() {
    Progress p;
    FILE* f = _wfopen(savePath().c_str(), L"rb");
    if (f == nullptr) return p;                      // 没有存档文件，就用全 0
    if (fscanf(f, "%d %d %d", &p.value[0], &p.value[1], &p.value[2]) != 3) p = Progress();
    fclose(f);
    return p;
}

void saveProgress(const Progress& p) {
    FILE* f = _wfopen(savePath().c_str(), L"wb");
    if (f == nullptr) return;
    fprintf(f, "%d %d %d\n", p.value[0], p.value[1], p.value[2]);
    fclose(f);
}

// 关卡阶数：通关 0~1 局是 2 阶，之后每两局升一阶，最高 10 阶
int levelOrder(int passed) {
    const int n = 2 + passed / 2;                    // 整数除法自动向下取整
    return n > 10 ? 10 : n;
}

wstring toWide(const string& s) { return wstring(s.begin(), s.end()); }   // UTF-8 → 宽字符

const wchar_t* modeName(Mode m) {
    if (m == Mode::Restore) return L"复原模式";
    if (m == Mode::Inverse) return L"求逆模式";
    return L"化简模式";
}

// 某一行的底色：当前行绿色、上一次选的行黄色、其余白色
COLORREF rowColor(int row, int current, int last) {
    if (row == current) return RGB(160, 240, 160);
    if (row == last) return RGB(255, 255, 210);
    return RGB(255, 255, 255);
}

class App {
public:
    void run();

private:
    void showLauncher();                                 // 画主菜单
    void startGame(Mode mode);                           // 开一局新游戏
    void buildGrid(const Matrix& mat, int originX, int originY, int n,
                   int cw, int ch, bool clickable, vector<HWND>& cells);
    void refresh();                                      // 按最新状态刷新所有格子
    void onWin();                                        // 通关后的处理

    ui::Window win_;
    Progress   progress_ = loadProgress();
    unique_ptr<Game> game_;
    Mode mode_ = Mode::Simplify;
    bool inGame_ = false;
    vector<HWND> mainCells_;                             // 左侧矩阵的格子
    vector<HWND> extraCells_;                            // 右侧矩阵的格子
};

void App::run() {
    if (!win_.create(L"线性代数小游戏（精简版）", 560, 420)) return;
    // 注册按键回调：菜单里按键直接忽略，游戏里 Ctrl+数字选行、其余交给 Game
    win_.onKey([this](wchar_t key, bool ctrl) {
        if (!inGame_ || !game_) return;
        if (ctrl) {
            int row = -1;
            if (key >= L'1' && key <= L'9') row = key - L'1';
            else if (key == L'0' && game_->n() >= 10) row = 9;   // 10 阶时 Ctrl+0 选第 10 行
            if (row >= 0 && row < game_->n()) { game_->selectRow(row); refresh(); }
            return;
        }
        if (game_->press(char(key))) {                   // 只有矩阵真的变了才重画
            refresh();
            if (game_->won()) onWin();
        }
    });
    showLauncher();
    ShowWindow(win_.handle(), SW_SHOW);
    ui::Window::runLoop();
}

void App::showLauncher() {
    inGame_ = false;
    game_.reset();
    win_.clear();
    win_.resize(560, 420);
    win_.setTitle(L"线性代数小游戏（精简版）");
    win_.addLabel(L"线性代数小游戏", 0, 26, 560, 40, 22);

    const int buttonW = 440;
    const int buttonX = (560 - buttonW) / 2;         // 水平居中
    for (int k = 0; k < 3; ++k) {
        const Mode m = Mode(k);
        const int passed = progress_[m];
        const wstring text = wstring(modeName(m)) + L"（" + to_wstring(levelOrder(passed)) +
                             L" 阶）　已通关 " + to_wstring(passed) + L" 局";
        win_.addButton(text, buttonX, 96 + k * 56, buttonW, 42,
                       [this, m] { win_.post([this, m] { startGame(m); }); });
    }
    win_.addButton(L"重置所有进度", (560 - 160) / 2, 96 + 3 * 56 + 14, 160, 36, [this] {
        win_.post([this] {
            if (ui::confirm(L"确定清空所有进度吗？", L"重置")) {
                progress_ = Progress();
                saveProgress(progress_);
                showLauncher();
            }
        });
    });
    win_.addLabel(L"化简：随机矩阵 → 单位矩阵", 20, 350, 520, 20, 10);
    win_.addLabel(L"复原：单位矩阵 → 目标矩阵", 20, 370, 520, 20, 10);
    win_.addLabel(L"求逆：用行变换求出逆矩阵", 20, 390, 520, 20, 10);
}

void App::buildGrid(const Matrix& mat, int originX, int originY, int n,
                    int cw, int ch, bool clickable, vector<HWND>& cells) {
    cells.assign(size_t(n) * n, nullptr);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            function<void()> onClick;
            if (clickable) onClick = [this, i] { game_->selectRow(i); refresh(); };  // 点格子选行
            const int index = i * n + j;             // 二维下标压成一维
            cells[index] = win_.addCell(originX + j * cw, originY + i * ch, cw, ch, 13, onClick);
            win_.setText(cells[index], toWide(mat[i][j].str()));
        }
    }
}

void App::startGame(Mode mode) {
    mode_ = mode;
    game_ = make_unique<Game>(mode, levelOrder(progress_[mode]));
    inGame_ = true;
    win_.clear();
    mainCells_.clear();
    extraCells_.clear();
    const int n = game_->n();
    const bool twoBoards = game_->twoBoards();       // 复原 / 求逆要显示两块矩阵
    const int cw = (mode == Mode::Simplify) ? 72 : 52;   // 单矩阵模式格子可以大一点
    const int ch = 42;
    const int gap = 32;                              // 两块矩阵之间的间距
    const int gridW = n * cw;                        // 一块矩阵的总宽度
    // 窗口尺寸 = 内容 + 边距；底部留 176 给按键提示和返回按钮
    int width = (twoBoards ? (2 * gridW + gap) : gridW) + 48;
    int height = n * ch + 176;
    if (width < 460) width = 460;                    // 小矩阵也给个最小窗口
    if (height < 420) height = 420;
    win_.resize(width, height);
    win_.setTitle(wstring(modeName(mode)) + L" - " + to_wstring(n) + L" 阶矩阵");

    const int originY = 56;                          // 网格顶部留出标题空间
    if (twoBoards) {
        const int leftX = (width - (2 * gridW + gap)) / 2;   // 两块矩阵整体水平居中
        buildGrid(game_->main(), leftX, originY, n, cw, ch, true, mainCells_);
        buildGrid(game_->extra(), leftX + gridW + gap, originY, n, cw, ch, false, extraCells_);
    } else {
        buildGrid(game_->main(), (width - gridW) / 2, originY, n, cw, ch, true, mainCells_);
    }
    // 底部两行常驻的按键提示（原版是弹窗，这里直接印在界面上，少一个窗口）
    win_.addLabel(L"Ctrl+数字 选行    w/s 上移下移    r 交换两行    q 相减    e 相加",
                  10, height - 82, width - 20, 20, 9);
    win_.addLabel(L"a 除以    d 乘以    f 取负    z 撤销    x 重做（连按 a/d 因子递增）",
                  10, height - 62, width - 20, 20, 9);
    win_.addButton(L"返回主界面", width - 126, height - 38, 106, 28,
                   [this] { win_.post([this] { showLauncher(); }); });
    refresh();
}

void App::refresh() {
    const int n = game_->n();
    // 复原模式右边的目标矩阵是固定的，不该跟着玩家选的行变色
    const bool colorExtra = (game_->mode() == Mode::Inverse);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            const int index = i * n + j;
            win_.setText(mainCells_[index], toWide(game_->main()[i][j].str()));
            win_.setBackColor(mainCells_[index], rowColor(i, game_->row(), game_->lastRow()));
            if (extraCells_.empty()) continue;       // 化简模式只有一块矩阵
            win_.setText(extraCells_[index], toWide(game_->extra()[i][j].str()));
            win_.setBackColor(extraCells_[index], colorExtra
                              ? rowColor(i, game_->row(), game_->lastRow()) : RGB(255, 255, 255));
        }
    }
}

void App::onWin() {
    const int n = game_->n();
    wstring message = L"恭喜！" + to_wstring(n) + L" 阶矩阵已化为单位矩阵。";
    if (mode_ == Mode::Restore) message = L"恭喜！" + to_wstring(n) + L" 阶矩阵已复原为目标矩阵。";
    if (mode_ == Mode::Inverse) message = L"左侧矩阵已化为单位阵，右侧就是它的逆矩阵。";
    ++progress_[mode_];
    saveProgress(progress_);
    ui::alert(message, L"胜利");
    const Mode next = mode_;
    win_.post([this, next] { startGame(next); });
}
}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    SetProcessDPIAware();                            // 自己按物理像素排版，免得被系统拉伸模糊
    srand((unsigned)time(nullptr));                  // 用当前时间初始化随机数种子
    App app;
    app.run();
    return 0;
}
