#pragma once

#include "GameState.h"
#include "GameEngine.h"
#include "GreedyBot.h"

#include <vector>

class GameCounters {
public:
    GameCounters(const GameEngine& ge, const GameState gs);
    
    // GameCounters(const std::vector<double>& vec);

    // std::vector<double> toVector() const;

    bool gameEnded = false;
    Resources productionResBalance;
    int curRound;
    double gameStage;

    int productionProfit;
    std::array<double, 60> questProgress;
    std::vector<int> buildings;
    std::vector<int> buildingsOnSale;
    
    std::vector<int> nWorkers;
    int workerWp;
    int buildingWp;
    int pureWp;
    int moneyWp;

    std::array<int, WpSourcesTotal> wpBySource;
};
