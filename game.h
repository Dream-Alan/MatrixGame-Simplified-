//  模块 3：game.h —— 游戏规则（纯逻辑，完全不碰界面）
//
//  三种模式看上去是三套玩法，其实是**同一个状态机**：
//      化简：左边随机可逆矩阵，目标 = 单位矩阵
//      复原：左边单位矩阵，     目标 = 右边那个随机矩阵
//      求逆：左边随机可逆矩阵， 右边同时有单位矩阵
//  把这层抽象出来，界面代码就只剩"画格子 + 转发按键"。
#pragma once

#include <vector>
using namespace std;

#include "matrix.h"

enum class Mode { Simplify, Restore, Inverse };          // 化简 / 复原 / 求逆

class Game {
public:
    Game(Mode mode, int n);                                          // 随机开一局新的
    Game(Mode mode, const Matrix& main, const Matrix& extra);         // 用指定矩阵开局（便于测试）
    Mode mode() const { return mode_; }
    int  n() const { return n_; }
    const Matrix& main() const { return main_; }           // 玩家在操作的那块矩阵
    const Matrix& extra() const { return extra_; }         // 复原：目标矩阵；求逆：右侧矩阵
    bool twoBoards() const { return mode_ != Mode::Simplify; }
    int row() const { return row_; }                       // 当前行，-1 表示还没选
    int lastRow() const { return lastRow_; }               // 上一次选的行
    void selectRow(int r);
    bool press(char key);                                  // 处理一个按键，返回矩阵是否变了
    bool undo();
    bool redo();
    bool won() const;

private:
    struct Snapshot { Matrix main, extra; int row, lastRow; };       // 一份"存档"：撤销时直接恢复它
    Snapshot grab() const;                                 // 抓一份当前状态
    void restore(const Snapshot& s);                       // 从快照恢复（撤销与重做共用）
    void save();                                           // 把当前状态压进历史栈
    void scaleBoth(int row, const Fraction& k);            // 下面三个都会同步作用于 extra
    void swapBoth(int i, int j);
    void addBoth(int dst, int src, const Fraction& k);
    void forgetRepeat();                                   // 清掉 a/d 连击记录

    Mode mode_;
    int  n_;
    Matrix main_;                                          // 左边（玩家操作的）矩阵
    Matrix extra_;                                         // 右边（目标 / 单位）矩阵
    vector<Snapshot> history_, redo_;                      // 撤销栈 / 重做栈
    int  row_ = -1;                                        // 当前行
    int  lastRow_ = -1;                                    // 上一次选的行
    char lastOp_ = 0;                                      // 上一次按的是 'a' 还是 'd'
    int  lastFactor_ = 0;                                  // 上一次用的因子，用于连击递增
    int  lastOpRow_ = -1;                                  // 上一次作用在哪一行
};
