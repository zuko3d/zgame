#pragma once

#include "GameRules.h"
#include "GameState.h"
#include "GameStats.h"
#include "Utils.h"
#include "Types.h"

#include <assert.h>
#include <vector>
#include <string>

class GameCounters;

using CountersDataset = std::vector<std::vector<std::pair<GameCounters, double>>>;

struct EngineTrainParams {
    std::array<double, WpSourcesTotal> sourceWeights;
    CountersDataset* gameCountersStorage = nullptr;
    int roundOfInterest = -1;
};

class GameEngine {
public:
    GameEngine(const GameRules& gameRules, std::vector<IBot*> bots_, const EngineTrainParams& etp_ = EngineTrainParams{})
        : rules(gameRules)
        , bots(std::move(bots_))
        , stats(bots.size())
        , etp(etp_)
    { }

    std::vector<int> playGame(int seed, bool withLogs = false, bool withStats = false);
    void doPreGameActions(GameState& gs, bool withLogs = false, bool withStats = false);
    void doTurn(GameState& gs, bool withLogs = false, bool withStats = false);
    void doAfterTurnActions(GameState& gs, bool withLogs = false, bool withStats = false) const;
    void advanceRound(GameState& gs) const;
    void scoreFinalPoints(GameState& gs, bool withStats) const;
    void awardWp(GameState& gs, int amount, WpSource source, bool withStats = false) const;

    Resources calcUpkeep(const GameState& gs) const;
    void payUpkeep(GameState& gs, bool withLogs = false, bool withStats = false) const;
    bool shouldPayUpkeep(const GameState& gs) const;

    void doAction(Action action, GameState& gs, bool withLogs = false, bool withStats = false) const;
    std::vector<Action> generateActions(const GameState& gs) const;

    // int getBuyPrice(Resources res) const;
    int getBuyPrice(Resources res, const Resources& market) const;
    int buyOrSellMats(Resources res, Resources& market) const;

    bool gameEnded(const GameState& gs) const;

    // const GameState& getGs() const {
    //     return gs;
    // }

    const GameRules& getRules() const {
        return rules;
    }

    // const Resources& getMarket() const {
    //     return gs.market;
    // }

    const std::vector<std::vector<std::vector<std::string>>>& getLog() const {
        return gameLog;
    }

    void resetLogsAndStats(int nPlayers);
    const GameStats& getStats() const {
        return stats;
    }
    GameStats getStatsCopy() const;

    std::string renderLogAsHtml() const;
    std::string renderPsAsHtml(const GameState& gs) const;
    std::string to_html(Action action, const GameState& gs) const;

private:
    void log(const std::string& s, const GameState& gs) const {
        gameLog[gs.activePlayer].back().emplace_back(s);
    }

    double findMaxPoints(const GameState& gs, const IBot* bot, int depth);
    std::vector<std::pair<GameCounters, double>> findChildrenMaxPoints(const GameState& gs, const IBot* bot, int depth);

    // GameState gs;
    const GameRules& rules;

    std::vector<IBot*> bots;

    mutable std::vector<std::vector<std::vector<std::string>>> gameLog;

    mutable GameStats stats;

    EngineTrainParams etp;
};
