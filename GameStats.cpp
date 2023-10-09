#include "GameStats.h"

#include "Utils.h"

#include <iomanip>
#include <sstream>
#include <unordered_map>

std::vector<double> GameResult::percentileWinPoints(const std::vector<GameResult> &gr, double pct) {
    
    int nBots = 0;
    for (const auto& r: gr) {
        for (const auto& idx: r.botIndices) {
            nBots = std::max(idx, nBots);
        }
    }
    nBots++;

    std::vector<std::vector<double>> games(nBots);
    for (const auto& res: gr) {
        for (const auto& [p, idx]: enumerate(res.botIndices)) {
            games.at(p).push_back(res.winPoints.at(idx));
        }
    }

    std::vector<double> ret(nBots);
    for (const auto& [g, idx]: enumerate(games)) {
        ret.at(idx) = percentile(g, pct);
    }

    return ret;
}

std::vector<double> GameResult::trueSkill(const std::vector<GameResult> &gr) {
    int nBots = 0;
    for (const auto& r: gr) {
        for (const auto& idx: r.botIndices) {
            nBots = std::max(idx, nBots);
        }
    }
    nBots++;

    std::vector<std::vector<double>> games(nBots);
    for (const auto& res: gr) {
        for (const auto& [p, idx]: enumerate(res.botIndices)) {
            games.at(p).push_back(res.winPoints.at(idx));
        }
    }

    std::vector<double> ret(nBots);
    for (const auto& [g, idx]: enumerate(games)) {
        ret.at(idx) = mean(g) - 4 * variance(g) / sqrt(g.size());
    }

    return ret;
}

std::vector<double> GameResult::avgWinPoints(const std::vector<GameResult> &gr) {
    std::vector<double> ret;
    std::vector<double> games;
    int nBots = 0;
    for (const auto& r: gr) {
        for (const auto& idx: r.botIndices) {
            nBots = std::max(idx, nBots);
        }
    }
    nBots++;

    ret.resize(nBots, 0.0);
    games.resize(nBots, 0.0);
    for (const auto& res: gr) {
        for (const auto& [p, idx]: enumerate(res.botIndices)) {
            games.at(p)++;
            ret.at(p) += res.winPoints.at(idx);
        }
    }

    for (const auto& [g, idx]: enumerate(games)) {
        ret.at(idx) /= g;
    }

    return ret;
}

std::vector<double> GameResult::winrates(const std::vector<GameResult> &gr) {
    std::vector<double> wins;
    std::vector<double> games;
    int nBots = 0;
    for (const auto& r: gr) {
        for (const auto& idx: r.botIndices) {
            nBots = std::max(idx, nBots);
        }
    }
    nBots++;

    wins.resize(nBots, 0.0);
    games.resize(nBots, 0.0);
    for (const auto& res: gr) {
        wins.at(res.botIndices.at(res.winner))++;
        for (const auto& [p, idx]: enumerate(res.botIndices)) {
            games.at(p)++;
        }
    }

    for (const auto& [g, idx]: enumerate(games)) {
        wins.at(idx) /= g;
    }

    return wins;
}

std::vector<PlayerIngameStats> GameResult::transposeAndFilter(const std::vector<GameResult>& results, int wpThreshold) {
    std::vector<PlayerIngameStats> ret;
    ret.reserve(results.size());

    for (const auto& gr: results) {
        if (gr.winPoints.at(gr.winner) >= wpThreshold) {
            ret.emplace_back(gr.stats.pis.at(gr.winner));
        }
    }

    return ret;
}
