#pragma once

#include "Resources.h"

#include <string>

struct WorkerCard {
    Resources price;
    std::vector<int> workerTypes;
    std::string htmlTag;

    // std::string to_string() {
    //     std::string ret;

    // }
};

struct Worker {
    int marketPower = 1;
    int productionPower = 1;
    int winPoints = 0;
    Resources price;
    int color;
    bool canGoToOpp = false;
};

struct WorkerUpgrade {
    int fromIdx;
    int toIdx;
    Resources price;
};
