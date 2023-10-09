#pragma once

#include "Resources.h"
#include "Utils.h"

#include <string>

struct Building {
    void tunePrice(double magnitude) {
        // magnitude > 1 makes price higher
        static std::default_random_engine rng(234);
        int totalChange = lnrand(rng) * (magnitude - 1) * sum(price.amount);
        while (totalChange > 0 && (sum(price.amount) > 1)) {
            int pos = rng() % price.amount.size();
            if (price.amount.at(pos) > 0) {
                price.amount.at(pos)++;
                totalChange--;
            }
        }
        while (totalChange < 0 && (sum(price.amount) > 1)) {
            int pos = rng() % price.amount.size();
            if (price.amount.at(pos) > 0) {
                price.amount.at(pos)--;
                totalChange++;
            }
        }
    }

    int winPoints = -123;
    Resources in, out, upkeep, price;
    int workerColor = -123;
    int maxWorkers = -123;
    double priority = -123;
    std::string htmlTag = "err";
    int color = -123;
    size_t idx = -123;
};