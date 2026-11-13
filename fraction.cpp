//  模块 1：fraction.cpp —— 精确有理数的实现
#include "fraction.h"

#include <numeric>                                       // gcd：求最大公约数

static long long absolute(long long v) { return v < 0 ? -v : v; }

Fraction::Fraction(long long num, long long den) : num_(num), den_(den) {
    if (den_ == 0) {                                     // 除零保护：游戏里不会走到，兜底而已
        num_ = 0;
        den_ = 1;
        return;
    }
    reduce();
}

// 约分：先把符号统一（分母不许为负），再除以最大公约数
void Fraction::reduce() {
    if (den_ < 0) {                                      // ① 负号一律挪到分子上
        num_ = -num_;
        den_ = -den_;
    }
    const long long g = gcd(absolute(num_), den_);       // ② 分子分母的最大公约数
    if (g > 1) {                                         // ③ 一起除掉，得到最简形式
        num_ /= g;
        den_ /= g;
    }
}

// 加法：先通分成 a/b + c/d = (ad + cb) / bd，再约分
Fraction& Fraction::operator+=(const Fraction& o) {
    num_ = num_ * o.den_ + o.num_ * den_;
    den_ *= o.den_;
    reduce();
    return *this;
}

Fraction& Fraction::operator-=(const Fraction& o) { return *this += (-o); }   // 减法 = 加相反数

Fraction& Fraction::operator*=(const Fraction& o) {
    num_ *= o.num_;
    den_ *= o.den_;
    reduce();
    return *this;
}

Fraction& Fraction::operator/=(const Fraction& o) {      // 除法 = 乘以倒数
    num_ *= o.den_;
    den_ *= o.num_;
    reduce();
    return *this;
}

string Fraction::str() const {
    if (den_ == 1) return to_string(num_);
    return to_string(num_) + "/" + to_string(den_);
}
