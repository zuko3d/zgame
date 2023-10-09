#pragma once

#include "Building.h"
#include "Workers.h"
#include "Quest.h"

#include <cstdint>
#include <string>

struct GameRules {
    GameRules(int);

    std::string renderRulesHtml() const;

    int tuneVersion = 0;

    int seed;
    uint8_t nResources = -1;

    uint8_t nWorkerGrades = -1;
    bool haveWorkerUpkeep = true;
    std::vector<Resources> workerUpkeepCost;
    std::vector<WorkerCard> workerCards;
    std::vector<Worker> workers;
    std::vector<const Worker*> workersByProductionPower;
    std::vector<WorkerUpgrade> workerUpgades;
    // std::vector<int> winPointsPerWorker;

    int thresholdType = -123;
    int thresholdParam = -123;
    std::vector<std::pair<int, int>> priceThresholds;
    Resources marketStartingResources;
    int marketMaxResources = -123;
    std::vector<int> pricePerAmount;
    int marketSaleTax = -123;
    int marketWorkerMoneyBonus = -123;
    int maxWorkersAtMarket = -123;
    double marketWpCoef = 1.0;

    int totalUpkeeps = -123;
    int stepsBetweenUpkeep = -123;
    int totalRounds = -123;

    int nBuildingGroups;
    // int foodBuildings = -123;
    bool haveBuildingUpkeep = true;
    std::vector<Building> basicBuildings;
    std::vector<Building> advancedBuildings;
    int nBuildingColors = -123;

    Resources playerStartingResources;
    int borrowValue = -123;

    int moneyPerWp = 3;

    int maxPlayersPerQuest = 1;
    std::vector<double> wpDiscountForQuest;
    std::vector<Quest> quests;
    int nQuestsInMatch;
};
