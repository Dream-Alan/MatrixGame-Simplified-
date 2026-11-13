//  模块 2：matrix.cpp —— 矩阵运算与初等行变换的实现
#include "matrix.h"

#include <cstdlib>                                       // rand()
#include <utility>                                       // swap()

// n 阶单位矩阵：先造全 0 方阵，再把主对角线改成 1
Matrix identity(int n) {
    Matrix m(n, vector<Fraction>(n));
    for (int i = 0; i < n; ++i) m[i][i] = Fraction(1);
    return m;
}

bool isIdentity(const Matrix& m) {
    const int n = int(m.size());
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            if (m[i][j] != Fraction(i == j ? 1 : 0)) return false;
    return true;
}

bool equal(const Matrix& a, const Matrix& b) {
    if (a.size() != b.size()) return false;
    for (int i = 0; i < int(a.size()); ++i)
        for (int j = 0; j < int(a[i].size()); ++j)
            if (a[i][j] != b[i][j]) return false;
    return true;
}

// 高斯消元求秩：一列一列往下扫，每列找一个"主元"，把该列其它位置消成 0，
// 最后数找到几个主元。元素是精确分数，不会出现"主元其实是 0.0000001"的误判。
int matrixRank(Matrix m) {                  // 参数按值传入 = 自动拷贝一份，内部随便改
    const int rows = int(m.size());
    const int cols = rows > 0 ? int(m[0].size()) : 0;
    int r = 0;                                       // r = 已经找到的主元个数
    for (int c = 0; c < cols && r < rows; ++c) {
        int pivot = -1;                              // 在第 c 列找第一个非零元素
        for (int i = r; i < rows; ++i) {
            if (!m[i][c].isZero()) { pivot = i; break; }
        }
        if (pivot < 0) continue;                     // 这一列全是 0，没有主元，跳过
        swap(m[r], m[pivot]);                        // 把主元所在行换到第 r 行
        scaleRow(m, r, Fraction(1) / m[r][c]);       // 把主元变成 1
        for (int i = 0; i < rows; ++i) {             // 把这一列的其它行都消成 0
            if (i != r && !m[i][c].isZero()) addRow(m, i, r, -m[i][c]);
        }
        ++r;
    }
    return r;
}

bool invertible(const Matrix& m) {
    const int n = int(m.size());
    for (int i = 0; i < n; ++i)
        if (int(m[i].size()) != n) return false;     // 不是方阵，一律不可逆
    return matrixRank(m) == n;
}

// 随机生成一个可逆矩阵：生成一个、检查可逆、不可逆就重来。元素取 -3..3（与原 Python 程序一致）
Matrix randomInvertible(int n) {
    for (;;) {
        Matrix m(n, vector<Fraction>(n));
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                m[i][j] = Fraction(rand() % 7 - 3);   // %7 得到 0..6，减 3 就是 -3..3
        if (invertible(m)) return m;
    }
}

// ---- 三种初等行变换，整个游戏就是它们的组合 ----
void scaleRow(Matrix& m, int row, const Fraction& k) {
    for (int j = 0; j < int(m[row].size()); ++j) m[row][j] *= k;
}

void swapRows(Matrix& m, int i, int j) { swap(m[i], m[j]); }

void addRow(Matrix& m, int dst, int src, const Fraction& k) {
    for (int j = 0; j < int(m[dst].size()); ++j) m[dst][j] += k * m[src][j];
}
