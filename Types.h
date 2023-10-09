#pragma once

#include "Utils.h"

#include <array>
#include <iostream>
#include <iomanip>
#include <random>
#include <string>
#include <sstream>
#include <vector>

constexpr int MeasurerLearnerDepth = 4;

enum class ActionType {
    BuyWorker,
    BuyWorkerForFree,
    Build,
    Produce, // + DeployWorker
    Borrow,
    DeployToMarket,
    UpgradeWorker,
    CreateQuarter,
};
constexpr int TotalActionTypes = 8;
inline std::string actionTypeToString(ActionType type) {
    switch (type) {
        case ActionType::BuyWorker:
            return "BuyWorker";
        case ActionType::BuyWorkerForFree:
            return "BuyWorkerForFree";
        case ActionType::Build:
            return "Build";
        case ActionType::Produce:
            return "Produce";
        case ActionType::Borrow:
            return "Borrow";
        case ActionType::DeployToMarket:
            return "DeployToMarket";
        case ActionType::UpgradeWorker:
            return "UpgradeWorker";
        case ActionType::CreateQuarter:
            return "CreateQuarter";
    }

    return "eerr";
}

struct Action {
    ActionType type;
    int param;
};

enum WpSource {
    MarketDeployWp,
    ProductionWp,
    QuestWp,
    FinalGoldWp,
    BuildingWp,
    WorkerWp,
    WpSourcesTotal,
};
inline std::string wpSourceToString(WpSource type) {
    switch (type) {
        case WpSource::MarketDeployWp:
            return "MarketDeployWp";
        case WpSource::ProductionWp:
            return "ProductionWp";
        case WpSource::QuestWp:
            return "QuestWp";
        case WpSource::FinalGoldWp:
            return "FinalGoldWp";
        case WpSource::BuildingWp:
            return "BuildingWp";
        case WpSource::WorkerWp:
            return "WorkerWp";
        case WpSource::WpSourcesTotal:
            return "WpSourcesTotal";
        default:
            break;
    }

    return "eerr";
}

struct PlayerIngameStats {
    std::vector<Action> actionsLog;
    std::array<std::array<int, TotalActionTypes>, TotalActionTypes> actionAfterActionStat{{ {{ 0 }} }};
};

struct RulesMetrics {
    // aka balance. No action diversity means there is only one strategy
    // double actionDiversity;
    // double wpSourceDiversity;

    double p1p2winrate;

    // how much mcts steps do you need to beat 100-steps and 1k-steps with 60% (80%?) winrate?
    // less = better
    double skillIntensity;
    int skillCapInMctsSteps;
    // double avgMctsComplexity;

    // more = better, different strategies = good
    double botsWinrateDispersion;
    // amount of strong bots as a part of total champions
    double botElitism;
    double matchupNonTransitivity;
};

struct MeasurerParams {
    MeasurerParams() {
        for (auto& q: questCoef) {
            q = 0.0;
        }
        for (auto& q: buildingCoef) {
            q = 0.0;
        }
        for (auto& q: resourceIncomeCoef) {
            q = 0.0;
        }
        for (auto& q: workerCoef) {
            q = 0.0;
        }
    }

    MeasurerParams(std::default_random_engine& rng)
        : incomeCoef(lnrand(rng) * 3 / 10.0)
        , moneyCoef(lnrand(rng) * 0.5 / 10.0)
        , wpPow(lnrand(rng) / 10.0)
        , incomePow(nrand(rng) / 10.0)
        // , questCoef(lnrand(rng) * 4)
        , buildingWpCoef(lnrand(rng) / 10.0)
        , workerWpCoef(lnrand(rng) / 10.0)
        , buildingOnSaleCoef(lnrand(rng) / 10.0)
    {
        for (auto& q: questCoef) {
            q = lnrand(rng) / questCoef.size() / 10.0;
        }
        for (auto& q: buildingCoef) {
            q = lnrand(rng) / buildingCoef.size() / 10.0;
        }
        for (auto& q: resourceIncomeCoef) {
            q = lnrand(rng) / resourceIncomeCoef.size() / 10.0;
        }
        for (auto& q: workerCoef) {
            q = lnrand(rng) / resourceIncomeCoef.size() / 10.0;
        }
    }

    std::string serialize() const {
        std::ostringstream ss;
        ss << incomeCoef << " ";
        ss << moneyCoef << " ";
        ss << wpPow << " ";
        ss << incomePow << " ";
        ss << buildingWpCoef << " ";
        ss << workerWpCoef << " ";

        ss << "q ";
        for (const auto& q: questCoef) {
            ss << q << " ";
        }

        ss << "b ";
        for (const auto& q: buildingCoef) {
            ss << q << " ";
        }
        ss << buildingOnSaleCoef << " ";

        ss << "r ";
        for (const auto& q: resourceIncomeCoef) {
            ss << q << " ";
        }

        ss << "w ";
        for (const auto& q: workerCoef) {
            ss << q << " ";
        }

        return ss.str();
    }

    MeasurerParams(std::string str) {
        std::istringstream ss(str);
        std::string tmp;
        ss >> incomeCoef;
        ss >> moneyCoef;
        ss >> wpPow;
        ss >> incomePow;
        ss >> buildingWpCoef;
        ss >> workerWpCoef;
        ss >> tmp; // q
        for (auto& q: questCoef) {
            ss >> q;
        }
        ss >> tmp; // b
        for (auto& q: buildingCoef) {
            ss >> q;
        }
        ss >> buildingOnSaleCoef;

        ss >> tmp; // r
        for (auto& q: resourceIncomeCoef) {
            ss >> q;
        }

        ss >> tmp; // w
        for (auto& q: workerCoef) {
            ss >> q;
        }
    }

    static constexpr size_t totalFields() { return 145; }

    // std::vector<>

    double incomeCoef = 0.0;
    double moneyCoef = 0.0;
    double wpPow = 1.0;
    double incomePow = 1.0;
    double buildingWpCoef = 0.0;
    double workerWpCoef = 0.0;
    std::array<double, 60> questCoef;
    std::array<double, 50> buildingCoef;
    double buildingOnSaleCoef = 0.0;
    std::array<double, 20> resourceIncomeCoef;
    std::array<double, 8> workerCoef;
};
