//  模块 3：game.cpp —— 游戏规则的实现
#include "game.h"

namespace {
const int kMaxHistory = 100;                         // 最多记 100 步，和原程序一致

// 开局用的矩阵：随机可逆，但不能恰好是单位矩阵（否则一开局就已经赢了）
Matrix busyMatrix(int n) {
    Matrix m;
    do { m = randomInvertible(n); } while (isIdentity(m));
    return m;
}
}  // namespace

// 随机开一局
Game::Game(Mode mode, int n) : mode_(mode), n_(n) {
    if (mode_ == Mode::Restore) {
        main_ = identity(n);                         // 复原：从单位矩阵出发，去凑目标矩阵
        extra_ = busyMatrix(n);
    } else {
        main_ = busyMatrix(n);                       // 化简 / 求逆：从随机矩阵出发，化成单位阵
        extra_ = identity(n);
    }
}

Game::Game(Mode mode, const Matrix& main, const Matrix& extra)     // 用指定矩阵开局（测试用）
    : mode_(mode), n_(int(main.size())), main_(main), extra_(extra) {}

// ---- 三个"带同步"的行变换 ----
// 求逆模式下左右两块必须一起变，这样左边化成单位阵时右边正好是逆矩阵
void Game::scaleBoth(int row, const Fraction& k) {
    scaleRow(main_, row, k);
    if (mode_ == Mode::Inverse) scaleRow(extra_, row, k);
}

void Game::swapBoth(int i, int j) {
    swapRows(main_, i, j);
    if (mode_ == Mode::Inverse) swapRows(extra_, i, j);
}

void Game::addBoth(int dst, int src, const Fraction& k) {
    addRow(main_, dst, src, k);
    if (mode_ == Mode::Inverse) addRow(extra_, dst, src, k);
}

// 把连击记录清零（按了别的键、换了行、撤销重做之后都要清）
void Game::forgetRepeat() { lastOp_ = 0; lastFactor_ = 0; lastOpRow_ = -1; }

Game::Snapshot Game::grab() const {                  // 抓一份当前状态（save / undo / redo 都要用）
    Snapshot s;
    s.main = main_;
    s.extra = extra_;
    s.row = row_;
    s.lastRow = lastRow_;
    return s;
}

void Game::save() {
    history_.push_back(grab());
    if (int(history_.size()) > kMaxHistory) history_.erase(history_.begin());   // 太老的丢掉
}

void Game::restore(const Snapshot& s) {
    main_ = s.main;
    extra_ = s.extra;
    row_ = s.row;
    lastRow_ = s.lastRow;
    forgetRepeat();
}

bool Game::undo() {
    if (history_.empty()) return false;              // 没得撤
    redo_.push_back(grab());                         // 先把当前状态存进重做栈
    restore(history_.back());                        // 再恢复上一步
    history_.pop_back();
    return true;
}

bool Game::redo() {
    if (redo_.empty()) return false;
    history_.push_back(grab());
    restore(redo_.back());
    redo_.pop_back();
    return true;
}

void Game::selectRow(int r) {
    if (r < 0 || r >= n_ || r == row_) return;       // 越界、或者点的就是当前行，什么都不做
    lastRow_ = row_;                                 // 原来的当前行降级成"上一次行"
    row_ = r;
    forgetRepeat();
}

bool Game::won() const {
    if (mode_ == Mode::Restore) return equal(main_, extra_);   // 复原：两块一样就赢
    return isIdentity(main_);                                  // 化简 / 求逆：左边是单位阵就赢
}

// ---- 按键总入口。返回值表示"矩阵到底有没有变"，界面靠它决定要不要重画 ----
bool Game::press(char key) {
    if (key == 'z') return undo();                   // 撤销
    if (key == 'x') return redo();                   // 重做
    if (key == 'a' || key == 'd') {                  // 缩放 + 连击
        if (row_ < 0) return false;                  // 还没选行
        save();
        // 这次算不算"连击"：上一次也是同一个键、并且还是同一行
        const bool repeated = (lastOp_ == key && lastOpRow_ == row_);
        const int factor = repeated ? lastFactor_ + 1 : 2;   // 连击则因子加一
        if (repeated) {
            // 关键：先把上一次的缩放撤回、回到原值，再按新因子缩放。
            // 若直接再缩放一次，就变成累乘（除以 2 再除以 3 等于除以 6），是错的。
            const Fraction back = (key == 'a') ? Fraction(lastFactor_) : Fraction(1, lastFactor_);
            scaleBoth(row_, back);
        }
        if (key == 'a') scaleBoth(row_, Fraction(1, factor));   // a：除以 factor
        else            scaleBoth(row_, Fraction(factor));      // d：乘以 factor
        lastOp_ = key;
        lastFactor_ = factor;
        lastOpRow_ = row_;
        return true;
    }
    if (row_ < 0) return false;                      // 下面这些操作都要先选一行
    if (key == 'w') {                                // 当前行上移
        if (row_ == 0) return false;                 // 已经在最上面了
        save();
        swapBoth(row_, row_ - 1);
        --row_;                                      // 行往上走了，当前行也要跟着走
    } else if (key == 's') {                         // 当前行下移
        if (row_ == n_ - 1) return false;            // 已经在最下面了
        save();
        swapBoth(row_, row_ + 1);
        ++row_;
    } else if (key == 'r') {                         // 和上一次选的行交换
        if (lastRow_ < 0 || lastRow_ == row_) return false;
        save();
        swapBoth(row_, lastRow_);                    // 矩阵的两行交换
        const int temp = row_;                       // 两个行号也互换角色
        row_ = lastRow_;
        lastRow_ = temp;
    } else if (key == 'q') {                         // 当前行减去上一次行
        if (lastRow_ < 0) return false;
        save();
        addBoth(row_, lastRow_, Fraction(-1));
    } else if (key == 'e') {                         // 当前行加上上一次行
        if (lastRow_ < 0) return false;
        save();
        addBoth(row_, lastRow_, Fraction(1));
    } else if (key == 'f') {                         // 当前行取负
        save();
        scaleBoth(row_, Fraction(-1));
    } else {
        forgetRepeat();                              // 按了没用的键，连击重新开始
        return false;
    }
    forgetRepeat();
    return true;
}
