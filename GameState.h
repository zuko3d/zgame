#pragma once

#include "IBot.h"
#include "Building.h"
#include "GameRules.h"
#include "PlayerState.h"
#include "Utils.h"
#include "Types.h"
#include "Workers.h"
#include "Quest.h"

#include <string>
#include <vector>

struct GameState {
    GameState() = delete;

    GameState(const GameState&) = default;

    GameState(const GameRules& rules, int nPlayers_, int seed) 
        : nPlayers(nPlayers_)
        , market(rules.marketStartingResources)
        , seed_(seed)
    {
        players.resize(nPlayers, rules);
        std::default_random_engine rng(seed);

        std::vector<int> buildings;
        for (int i = 0; i < rules.basicBuildings.size(); ++i) {
            buildings.emplace_back(i);
        }
        rshuffle(buildings, rng);
        basicBuildings.resize(rules.nBuildingGroups);

        for (int i = 0; i < rules.basicBuildings.size(); ++i) {
            basicBuildings[i % rules.nBuildingGroups].emplace_back(
                &rules.basicBuildings[buildings[i]]
            );
        }
        for (auto& bb: basicBuildings) {
            std::sort(bb.begin(), bb.end(), 
                [](const Building* op1, const Building* op2) { return op1->priority <= op2->priority; });
        }

        std::vector<int> qidx;
        for (const auto& [q, idx]: enumerate(rules.quests)) {
            qidx.push_back(idx);
        }
        rshuffle(qidx, rng);
        qidx.resize(rules.nQuestsInMatch);

        for (const auto& idx: qidx) {
            questProgress.emplace_back(std::pair<const Quest&, std::vector<int>>(
                rules.quests.at(idx), {}));
        }
    }

    const PlayerState& getPS() const {
        return players[activePlayer];
    }

    // std::string dump() const {
    //     throw std::runtime_error("dump is not implemented!");
    //     return "";
    // }

    std::vector<PlayerState> players;

    int activePlayer = 0;
    int nPlayers;
    int currentRound = 0;
    Resources market;

    std::vector<std::vector<const Building*>> basicBuildings;
    std::vector<std::pair<const Quest&, std::vector<int>>> questProgress;
    int seed_ = -123;
};
