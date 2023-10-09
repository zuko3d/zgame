#pragma once

#include "Utils.h"

#include <random>
#include <string>
#include <vector>

enum RT {
    WinPoints = 0,
    Money = 1,
};

struct Resources {
    Resources() { };

    Resources(int n)
        : amount(n ,0)
    { }

    void clip(int min, int max) {
        for (auto& amnt: amount) {
            if (amnt > max) amnt = max;
            else if (amnt < min) amnt = min;
        }
    }

    Resources(int total, size_t rStart, size_t rEnd, int quantity) {
        amount.resize(total, 0);
        static std::default_random_engine rng(34);

        for (int i = 0; i < quantity; i++) {
            amount[rStart + rng() % (rEnd - rStart)]++;
        }
    }
    Resources(int total, size_t rStart, size_t rEnd, Resources banned, int quantity) {
        amount.resize(total, 0);
        static std::default_random_engine rng(34);

        for (int i = 0; i < quantity; i++) {
            int res = rStart + rng() % (rEnd - rStart);
            while (banned.amount[res] > 0) {
                res = rStart + rng() % (rEnd - rStart);
            }
            amount[res]++;
        }
    }

    Resources(int total, size_t rStart, size_t rEnd, int minAmount, int maxAmount) {
        amount.resize(total, 0);
        static std::default_random_engine rng(35);

        int ceil = maxAmount - minAmount + 1;
        for (int i = rStart; i < rEnd; i++) {
            amount[i] = minAmount + rng() % ceil;
        }
    }

    void operator+=(const Resources& rhs) {
        for (int i = 0; i < amount.size(); i++) {
            amount[i] += rhs.amount[i]; 
        }
    }

    void operator-=(const Resources& rhs) {
        for (int i = 0; i < amount.size(); i++) {
            amount[i] -= rhs.amount[i]; 
        }
    }

    void operator*=(const Resources& rhs) {
        for (int i = 0; i < amount.size(); i++) {
            amount[i] *= rhs.amount[i]; 
        }
    }

    void operator*=(int scale) {
        for (int i = 0; i < amount.size(); i++) {
            amount[i] *= scale; 
        }
    }

    Resources operator+(const Resources& rhs) const {
        auto ret = *this;
        ret += rhs;

        return ret;
    }

    Resources operator-(const Resources& rhs) const {
        auto ret = *this;
        ret -= rhs;

        return ret;
    }

    Resources operator*(const Resources& rhs) const {
        auto ret = *this;
        ret *= rhs;

        return ret;
    }

    Resources operator*(int scalar) const {
        auto ret = *this;
        for (int i = 0; i < amount.size(); i++) {
            ret.amount[i] *= scalar; 
        }
        return ret;
    }

    int dot(const Resources&rhs) const {
        int ret = 0;
        for (int i = 0; i < amount.size(); i++) {
            ret += amount[i] * rhs.amount[i]; 
        }

        return ret;
    }

    std::string renderHtml() const {
        std::string ret;
        ret.reserve(200);

        ret += "<table><tr><td>";
        for (const auto& [amnt, idx]: enumerate(amount)) {
            if (amnt != 0) {
                ret += "<table><tr><td><center>" + std::to_string(amnt) + "</center></td></tr><tr><td>" +
                    "<img height=60px width=60px src =\"images/r" + std::to_string(idx) + ".png\">" +
                    "</td></tr></table></td><td>";
            }
        }

        ret += "</td></tr></table>";
        return ret;
    }

    std::string renderHtmlWithPrice(const std::vector<int>& pricePerAmount) const {
        std::string ret;
        ret.reserve(200);

        ret += "<table><tr><td>";
        for (const auto& [amnt, idx]: enumerate(amount)) {
            ret += "<table><tr><td><center>" + std::to_string(amnt) + "(" + std::to_string(pricePerAmount[amnt]) + "g)</center></td></tr><tr><td>" +
                "<img height=60px width=60px src =\"images/r" + std::to_string(idx) + ".png\">" +
             "</td></tr></table></td><td>";
        }

        ret += "</td></tr></table>";
        return ret;
    }

    std::vector<int> amount;
};
