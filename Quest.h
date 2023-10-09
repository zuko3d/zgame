#pragma once

#include <vector>

struct PlayerState;

enum class QuestRequirementType {
    UniqueBuildingColors,
    ExactBuildingColorAmount,
    TotalBuildingsAmount,
    UniqueWorkerTypes,
    ExactWorkerTypeAmount,
    TotalWorkersAmount,
    GoldAmount,
};

struct QuestRequirement {
    QuestRequirementType type;
    int paramType;
    int paramAmount;
};

struct Quest {
    std::vector<QuestRequirement> reqs;
    int winPoints;
    const int idx = -1;

    double checkProgress(const PlayerState& ps) const;
};