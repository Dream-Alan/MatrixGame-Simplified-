//  核心逻辑测试（不涉及界面）—— build.bat test
//  只用到 fraction / matrix / game 三个模块，所以游戏规则能在命令行里完整验证。
#include <cstdio>

#include "../fraction.h"
#include "../game.h"
#include "../matrix.h"

static int g_pass = 0, g_fail = 0;
static void check(bool ok, const char* what) {
    if (ok) ++g_pass;
    else { ++g_fail; std::printf("FAIL: %s\n", what); }
}
static Matrix make2(long long a, long long b, long long c, long long d) {
    return Matrix{{Fraction(a), Fraction(b)}, {Fraction(c), Fraction(d)}};
}

int main() {
    // ---- 分数：四则运算、约分、显示 ----
    check(Fraction(1, 2) + Fraction(1, 3) == Fraction(5, 6), "1/2 + 1/3 = 5/6");
    check(Fraction(2, 4).str() == "1/2", "2/4 自动约分成 1/2");
    check(Fraction(3, -6).str() == "-1/2", "分母为负时把负号提到分子");
    check(Fraction(1, 2) * Fraction(2, 3) == Fraction(1, 3), "1/2 * 2/3 = 1/3");
    check(Fraction(1, 2) / Fraction(1, 4) == Fraction(2), "1/2 ÷ 1/4 = 2");
    check(Fraction(5) - Fraction(5) == Fraction(0), "5 - 5 = 0");
    check(Fraction(3) == Fraction(6, 2), "3 和 6/2 是同一个数");
    // ---- 矩阵：单位阵、秩、可逆性 ----
    check(isIdentity(identity(3)), "单位矩阵判定");
    check(matrixRank(identity(4)) == 4, "4 阶单位矩阵满秩");
    check(matrixRank(make2(1, 2, 2, 4)) == 1, "两行成比例的矩阵秩为 1");
    check(!invertible(make2(1, 2, 2, 4)) && invertible(make2(1, 2, 3, 4)), "秩不满则不可逆");
    for (int t = 0; t < 20; ++t) check(invertible(randomInvertible(5)), "随机 5 阶矩阵一定可逆");
    // ---- 三种初等行变换 ----
    Matrix m = identity(2);
    swapRows(m, 0, 1);
    check(m[0][1] == Fraction(1), "交换两行");
    m = identity(2);
    addRow(m, 0, 1, Fraction(2));
    check(m[0][1] == Fraction(2), "第 0 行 += 2 × 第 1 行");
    m = identity(2);
    scaleRow(m, 0, Fraction(1, 3));
    check(m[0][0] == Fraction(1, 3), "第 0 行 × 1/3");
    // ---- 选行 ----
    Game g1(Mode::Simplify, make2(1, 2, 3, 4), identity(2));
    check(g1.row() == -1, "开局没有选中行");
    g1.selectRow(0);
    g1.selectRow(0);
    check(g1.row() == 0, "重复选同一行不改变状态");
    g1.selectRow(1);
    check(g1.row() == 1 && g1.lastRow() == 0, "选新行后，旧行成为“上一次行”");
    // ---- a / d 连击：先乘回上一次的因子，再按新因子缩放 ----
    Game g2(Mode::Simplify, make2(2, 0, 0, 1), identity(2));
    g2.selectRow(0);
    check(g2.press('a') && g2.main()[0][0] == Fraction(1), "按 a 把当前行除以 2");
    g2.press('a');
    check(g2.main()[0][0] == Fraction(2, 3), "连按 a 因子升到 3（先撤销上一次再缩放）");
    // ---- 撤销 / 重做 ----
    Game g3(Mode::Simplify, make2(2, 0, 0, 1), identity(2));
    g3.selectRow(0);
    g3.press('a');
    check(g3.undo() && g3.main()[0][0] == Fraction(2), "撤销回到除以 2 之前");
    check(g3.redo() && g3.main()[0][0] == Fraction(1), "重做再次除回去");
    check(g3.redo() == false, "重做栈空了以后再按 x 什么都不做");
    // ---- 化简：[[1,2],[0,1]] 用两次"减去上一行"化成单位阵 ----
    Game g4(Mode::Simplify, make2(1, 2, 0, 1), identity(2));
    check(!g4.won(), "化简模式开局不算赢");
    g4.selectRow(1);
    g4.selectRow(0);
    g4.press('q');
    check(!g4.won(), "只减一次还没到单位阵");
    g4.press('q');
    check(g4.won() && isIdentity(g4.main()), "减两次得到单位阵 → 胜利");
    // ---- 复原：单位阵把第 1 行乘 2，追上目标 [[1,0],[0,2]] ----
    Game g5(Mode::Restore, identity(2), make2(1, 0, 0, 2));
    check(!g5.won(), "复原模式开局和目标不相同");
    g5.selectRow(1);
    g5.press('d');
    check(g5.won(), "当前矩阵与目标矩阵相等 → 胜利");
    // ---- 求逆：对 [[1,2],[0,1]] 做行变换，右侧单位阵同步变成逆矩阵 ----
    Game g6(Mode::Inverse, make2(1, 2, 0, 1), identity(2));
    g6.selectRow(1);
    g6.selectRow(0);
    g6.press('q');
    g6.press('q');
    check(g6.won(), "求逆模式左侧化为单位阵 → 胜利");
    check(equal(g6.extra(), make2(1, -2, 0, 1)), "右侧矩阵就是原矩阵的逆矩阵");

    std::printf("\n通过 %d 项，失败 %d 项\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
