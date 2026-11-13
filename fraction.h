//  模块 1：fraction.h —— 精确有理数
//
//  "分子 + 分母"都在整数里走的分数类型，保证全程精确。
#pragma once

#include <string>
using namespace std;                                     // 为了好读，本工程统一省掉 std:: 前缀

class Fraction {
public:
    Fraction() : num_(0), den_(1) {}                     // 默认值是 0
    Fraction(long long n) : num_(n), den_(1) {}          // 整数可以隐式转换成分数
    Fraction(long long num, long long den);              // 自动约分，负号一律放到分子上
    long long num() const { return num_; }               // 读分子
    long long den() const { return den_; }               // 读分母（恒大于 0）
    bool isZero() const { return num_ == 0; }            // 是不是 0
    string str() const;                                  // 显示用："3" 或 "-1/2"
    Fraction operator-() const { return Fraction(-num_, den_); }   // 取相反数

    // 赋值版真正干活，friend 版先拷贝一份再调用赋值版，这样每种运算只写一份逻辑
    Fraction& operator+=(const Fraction& o);
    Fraction& operator-=(const Fraction& o);
    Fraction& operator*=(const Fraction& o);
    Fraction& operator/=(const Fraction& o);
    friend Fraction operator+(Fraction a, const Fraction& b) { return a += b; }
    friend Fraction operator-(Fraction a, const Fraction& b) { return a -= b; }
    friend Fraction operator*(Fraction a, const Fraction& b) { return a *= b; }
    friend Fraction operator/(Fraction a, const Fraction& b) { return a /= b; }
    // 因为约分过、且分母为正，两个分数相等 ⇔ 分子分母分别相等
    friend bool operator==(const Fraction& a, const Fraction& b) {
        return a.num_ == b.num_ && a.den_ == b.den_;
    }
    friend bool operator!=(const Fraction& a, const Fraction& b) { return !(a == b); }

private:
    void reduce();                                       // 约分 + 规范符号
    long long num_;                                      // 分子
    long long den_;                                      // 分母，恒 > 0
};
