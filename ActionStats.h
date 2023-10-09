#pragma once

#include "GameStats.h"

#include <string>
#include <vector>

class ActionStats {
public:
    ActionStats(const std::vector<GameResult> &gr, double wpThresholdPercentile = 0.8, bool onlyWinner = true);

    std::string serialize() const;

    int totalGames = 0;
    std::vector<double> buildingAmount; // we shouldn't have more than 100 buildings;
    std::vector<double> questAmount; // we shouldn't have more than 100 quests;
    
    std::vector<double> actionPct01;
    std::vector<double> actionPct09;
    std::vector<double> wpSourcePct01;
    std::vector<double> wpSourcePct09;
};
