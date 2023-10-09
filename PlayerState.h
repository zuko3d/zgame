#pragma once

#include "Resources.h"
#include "GameRules.h"
#include "Types.h"

struct GameState;
class GameEngine;

#include <vector>

struct PlayerState {
    PlayerState(const GameRules& rules);

    Resources resources;
    std::vector<const Building*> buildings;
    std::vector<int> nWorkers;

    void updateUniqueWorkerTypes();
    void updateUniqueBuildingColors();
    void updateBuildingsByColor();
    void updateQuestProgress(const GameEngine& ge, const GameState& gs);

    int uniqueWorkerTypes = 0;
    int uniqueBuildingColors = 0;
    std::vector<int> buildingsByColor;

    // std::vector<int> goodsProduced;

    std::vector<double> questProgress;

    std::array<int, WpSourcesTotal> wpSources{{0}};
};