//  模块 2：matrix.h —— 矩阵与初等行变换
//
//  每个按键都只是调用其中一两个函数。
#pragma once

#include <vector>
using namespace std;

#include "fraction.h"

typedef vector<vector<Fraction>> Matrix;                 // 类型别名，写起来短

Matrix identity(int n);                                  // n 阶单位矩阵
bool isIdentity(const Matrix& m);                        // 是不是单位矩阵
bool equal(const Matrix& a, const Matrix& b);            // 两个矩阵是否完全相同
int  matrixRank(Matrix m);                               // 矩阵的秩（内部会改 m，所以按值传）
bool invertible(const Matrix& m);                        // 是否可逆 ⇔ 方阵且满秩
Matrix randomInvertible(int n);                          // 随机生成一个可逆矩阵

// ---- 三种初等行变换，整个游戏就是它们的组合 ----
void scaleRow(Matrix& m, int row, const Fraction& k);    // 第 row 行整体乘以 k
void swapRows(Matrix& m, int i, int j);                  // 交换第 i、j 两行
void addRow(Matrix& m, int dst, int src, const Fraction& k);   // 第 dst 行 += k × 第 src 行
