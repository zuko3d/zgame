#pragma once

#include "ActionStats.h"
#include "IBot.h"
#include "GameRules.h"
#include "Types.h"
#include "GameCounters.h"

#include <vector>
#include <unordered_map>

class Tournament {
public:
    static GameResult playSingleGame(const GameRules& rules, const std::vector<IBot*>& bots, int seed, bool withLogs = false);
    static std::vector<GameResult> playAllInAll(const GameRules& rules, const std::vector<IBot*>& bots, int repeat, int timeLimit = 20);
    static std::pair<std::vector<double>, std::vector<double>> playTeamVTeam(const GameRules& rules, const std::vector<IBot*>& bots1, const std::vector<IBot*>& bots2, int repeat);

    static std::vector<GameResult> playSP(const EngineTrainParams& etp, const GameRules& rules, const std::vector<IBot*>& bots, int repeat, int seedStart = 0, bool withStats = false, int timeLimitSeconds = 60);

    static ActionStats measureRules(const GameRules& rules, const std::vector<IBot*>& bots, int repeat, int seedStart = 1000000);
};
