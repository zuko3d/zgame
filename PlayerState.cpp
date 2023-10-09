#include "PlayerState.h"

#include "GameState.h"
#include "GameEngine.h"
#include "Utils.h"

#include <algorithm>

PlayerState::PlayerState(const GameRules& rules) 
    : resources(rules.playerStartingResources)
    , nWorkers(rules.nWorkerGrades, 0)
    , buildingsByColor(rules.nBuildingColors, 0)
    , questProgress(rules.nQuestsInMatch, 0.0)
{
    for (auto& wps: wpSources) {
        wps = 0;
    }
}

void PlayerState::updateUniqueWorkerTypes() {
    uniqueWorkerTypes =countNonZero(nWorkers);
}

void PlayerState::updateUniqueBuildingColors() {
    uniqueBuildingColors = countNonZero(buildingsByColor);
}

void PlayerState::updateBuildingsByColor() {
    for (auto& c: buildingsByColor) {
        c = 0;
    }

    for (const auto& b: buildings) {
        buildingsByColor.at(b->color)++;
    }
}

void PlayerState::updateQuestProgress(const GameEngine& ge, const GameState& gs) {
    const int ap = gs.activePlayer;

    for (int qidx = 0; qidx < questProgress.size(); qidx++) {
        const auto& [q, pindices] = gs.questProgress.at(qidx);
        if (questProgress.at(qidx) >= 1.0) {
            assert((questProgress.at(qidx) <= 1.0) || is_in(ap, pindices));
            continue;
        } else {
            if (pindices.size() < ge.getRules().maxPlayersPerQuest) {
                questProgress.at(qidx) = q.checkProgress(*this);
            } else {
                questProgress.at(qidx) = 0.0;
            }
        }
    }
}
