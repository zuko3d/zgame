#pragma once

#include "PlayerState.h"
#include "Types.h"

#include <vector>

class GameStats {
public:
    GameStats(int nPlayers)
        : pis(nPlayers)
    { }
    
    std::vector<PlayerIngameStats> pis;
    std::vector<std::array<int, WpSourcesTotal>> wpSources;
    std::vector<PlayerState> playerFinalState;
};

struct GameResult
{
    static std::vector<double> avgWinPoints(const std::vector<GameResult> &gr);
    static std::vector<double> winrates(const std::vector<GameResult> &gr);
    static std::vector<double> percentileWinPoints(const std::vector<GameResult> &gr, double pct);
    static std::vector<double> trueSkill(const std::vector<GameResult> &gr);

    static std::vector<PlayerIngameStats> transposeAndFilter(const std::vector<GameResult>& results, int wpThreshold = 0);

	int winner;
	std::vector<int> winPoints;
    GameStats stats;
    std::vector<int> botIndices;
};
