#include "GameEngine.h"

#include "Timer.h"
#include "GameCounters.h"
#include "MctsBot.h"

#include <string>
#include <assert.h>

const std::string OPEN_TABLE = "<table><tr><td>";
const std::string CLOSE_TABLE = "</td></tr></table>";
const std::string TABLE_NEW_LINE{"</td></tr><tr><td>"};

std::string GameEngine::to_html(Action action, const GameState& gs) const {
    std::string ret;
    switch (action.type) {
        case ActionType::BuyWorker: {
            ret += "hire<br>" + rules.workerCards.at(action.param).htmlTag;
            break;
        };
        case ActionType::BuyWorkerForFree: {
            ret += "hire for free<br>" + rules.workerCards.at(action.param).htmlTag;
            break;
        };
        case ActionType::Build: {
            ret += "build<br>" + gs.basicBuildings.at(action.param).back()->htmlTag;
            break;
        };
        case ActionType::Produce: {
            ret += "produce " + std::to_string(action.param);
            break;
        };
        case ActionType::Borrow: {
            ret += "borrow " + std::to_string(action.param);
            break;
        };
        case ActionType::DeployToMarket: {
            ret += "DeployToMarket " + std::to_string(action.param);
            break;
        };
        case ActionType::UpgradeWorker: {
            ret += "UpgradeWorker " + std::to_string(action.param);
            break;
        };
        case ActionType::CreateQuarter: {
            ret += "CreateQuarter " + std::to_string(action.param);
            break;
        };
    }

    return ret;
}


void advanceToNextNode(const Action& action, const GameEngine& ge, GameState& gs) {
    ge.doAction(action, gs);
    ge.doAfterTurnActions(gs);
    ge.advanceRound(gs);
    if (ge.gameEnded(gs)) {
        ge.scoreFinalPoints(gs, false);
    }
}


double GameEngine::findMaxPoints(const GameState& gs, const IBot* bot, int depth) {
    const auto actions = generateActions(gs);
    double maxPts = -1;
    for (const auto& action: actions) {
        GameState newGs(gs);
        advanceToNextNode(action, *this, newGs);

        if (gameEnded(newGs) || (depth <= 1)) {
            maxPts = std::max(maxPts, bot->measureGs(etp.sourceWeights, GameCounters{*this, newGs}));
            // result.push_back({GameCounters(ge, newGs), newGs.players.front().resources.amount[WinPoints]});
        } else {
            maxPts = std::max(maxPts, findMaxPoints(newGs, bot, depth - 1));
        }

        if (gameEnded(newGs)) {
            assert(maxPts >= 0);
        }
    }

    return maxPts;
}

std::vector<std::pair<GameCounters, double>> GameEngine::findChildrenMaxPoints(const GameState& gs, const IBot* bot, int depth) {
    const auto actions = generateActions(gs);
    std::vector<std::pair<GameCounters, double>> ret;
    ret.reserve(actions.size());

    for (const auto& action: actions) {
        GameState newGs(gs);
        advanceToNextNode(action, *this, newGs);

        double childPts = -123.0;
        if (gameEnded(newGs) || (depth <= 1)) {
            childPts = bot->measureGs(etp.sourceWeights, GameCounters{*this, newGs});
            // result.push_back({GameCounters(ge, newGs), newGs.players.front().resources.amount[WinPoints]});
        } else {
            childPts = findMaxPoints(newGs, bot, depth - 1);
        }

        assert(std::isfinite(childPts));
        // assert(childPts >= 0);
        ret.emplace_back(GameCounters(*this, newGs), childPts);
    }

    return ret;
}

int buildAllStates(const GameEngine& ge, const GameState& gs, std::vector<std::pair<GameCounters, int>>& result) {
    const auto actions = ge.generateActions(gs);
    int maxPts = -1;
    for (const auto& action: actions) {
        GameState newGs(gs);
        advanceToNextNode(action, ge, newGs);

        if (ge.gameEnded(newGs)) {
            maxPts = std::max(maxPts, newGs.players.front().resources.amount[WinPoints]);
            // result.push_back({GameCounters(ge, newGs), newGs.players.front().resources.amount[WinPoints]});
        } else {
            maxPts = std::max(maxPts, buildAllStates(ge, newGs, result));
        }
    }
    result.push_back({GameCounters(ge, gs), maxPts});
    return maxPts;
}


void GameEngine::doPreGameActions(GameState& gs, bool withLogs, bool withStats) {
    const auto workerAction = bots.at(gs.activePlayer)->chooseStartingWorkerCard(*this, gs);
    doAction(Action{ActionType::BuyWorkerForFree, (int) workerAction}, gs, withLogs, withStats);
}

void GameEngine::scoreFinalPoints(GameState& gs, bool withStats) const {
    const auto& ps = gs.players.at(gs.activePlayer);

    int score = 0;
    for (const auto& b: ps.buildings) {
        score += b->winPoints;
    }
    awardWp(gs, score, BuildingWp, withStats);

    score = 0;
    for (const auto& [w, idx]: enumerate(ps.nWorkers)) {
        score += w * rules.workers.at(idx).winPoints;
    }
    awardWp(gs, score, WorkerWp, withStats);

    awardWp(gs, ps.resources.amount[Money] / rules.moneyPerWp, FinalGoldWp, withStats);

    for (const auto& [q, pindices]: gs.questProgress) {
        const auto pos = std::find(pindices.begin(), pindices.end(), gs.activePlayer);
        if (pos != pindices.end()) {
            awardWp(gs, q.winPoints * rules.wpDiscountForQuest.at(std::distance(pindices.begin(), pos)), QuestWp, withStats);
        }
    }

    assert(sum(ps.wpSources) == ps.resources.amount.at(WinPoints));
}

std::vector<int> GameEngine::playGame(int seed, bool withLogs, bool withStats) {
    int nPlayers = bots.size();
    auto gs = GameState(rules, nPlayers, seed);
    // Timer gameTimer;

    resetLogsAndStats(nPlayers);

    for (int p = 0; p < nPlayers; p++) {
        if (withLogs) {
            gameLog.at(p).emplace_back(std::vector<std::string>());
        }
        gs.activePlayer = p;
        if (withLogs) {
            log(renderPsAsHtml(gs), gs);
        }
        doPreGameActions(gs, withLogs, withStats);
    }

    gs.activePlayer = 0;
    while (!gameEnded(gs)) {
        if ((etp.gameCountersStorage != nullptr) && (gs.currentRound == etp.roundOfInterest)) {
            Timer rtimer;
            // std::vector<std::pair<GameCounters, int>> intermediateCounters;
            // intermediateCounters.reserve(1000000);
            // buildAllStates(*this, gs, intermediateCounters);
// #pragma omp critical (counters_insertion)
            // std::copy(intermediateCounters.begin(), intermediateCounters.end(), std::back_inserter(*gameCountersStorage));
            const auto childCounters = findChildrenMaxPoints(gs, bots.at(gs.activePlayer), MeasurerLearnerDepth + 1);
            // assert(std::isfinite(pts));
#pragma omp critical (counters_insertion)
            etp.gameCountersStorage->emplace_back(std::move(childCounters));

            break;

            // std::cout << "total moves: " << result.size() << ", possible best: " << result.back().second << std::endl;
            // std::cout << "packed states in " << rtimer.elapsedMilliSeconds() << " millis" << std::endl;
        }

        
        gameLog.at(gs.activePlayer).emplace_back(std::vector<std::string>());
        if (withLogs) {
            log(renderPsAsHtml(gs), gs);
        }
        doTurn(gs, withLogs, withStats);

        doAfterTurnActions(gs, withLogs, withStats);

        if (withLogs) {
            log(gs.market.renderHtmlWithPrice(rules.pricePerAmount), gs);
        }
    }

    std::vector<int> scores(nPlayers, 0);
    for (int p = 0; p < nPlayers; p++) {
        gs.activePlayer = p;
        scoreFinalPoints(gs, withStats);
        scores.at(p) = gs.players.at(p).resources.amount[WinPoints];
        assert(scores.at(p) >= 0);
    }

    if (withStats) {
        for (const auto& ps: gs.players) {
            stats.wpSources.emplace_back(ps.wpSources);
            stats.playerFinalState.push_back(ps);
        }
    }

    // std::cout << "actually got: " << scores.at(0) << std::endl;
    // std::cout << "match in " << gameTimer.elapsedMilliSeconds() << std::endl;
    return scores;
}

void GameEngine::doTurn(GameState& gs, bool withLogs, bool withStats) {
    Action action = bots[gs.activePlayer]->chooseAction(*this, gs);
    // log(to_html(action, gs), gs);
    doAction(action, gs, withLogs, withStats);
}

void GameEngine::doAfterTurnActions(GameState& gs, bool withLogs, bool withStats) const {
    const int ap = gs.activePlayer;
    auto& ps = gs.players.at(gs.activePlayer);
    for (const auto& [progress, qidx]: enumerate(ps.questProgress)) {
        if (progress > 0.999 && progress < 1.01) {
            auto& pindices = gs.questProgress.at(qidx).second;
            assert (pindices.size() < rules.maxPlayersPerQuest && !is_in(ap, pindices));
            pindices.push_back(ap);
            ps.questProgress.at(qidx) = 1.02;
        }
    }

    if (shouldPayUpkeep(gs)) {
        payUpkeep(gs, withLogs, withStats);
    }

    advanceRound(gs);
}

void GameEngine::advanceRound(GameState& gs) const {
    ++gs.activePlayer;
    if (gs.activePlayer >= gs.players.size()) {
        gs.activePlayer = 0;
        gs.currentRound++;
    }
}

void GameEngine::payUpkeep(GameState& gs, bool withLogs, bool withStats) const {
    auto& ps = gs.players[gs.activePlayer];
    Resources totalUpkeep(rules.nResources);
    for (const auto& b: ps.buildings) {
        totalUpkeep += b->upkeep;
    }
    for (const auto& [w, idx]: enumerate(ps.nWorkers)) {
        totalUpkeep +=  rules.workerUpkeepCost.at(idx) * w;
    }

    int toPay = buyOrSellMats(totalUpkeep, gs.market);
    ps.resources.amount[Money] -= toPay;

    if (withLogs) {
        log("Paid upkeep: " + std::to_string(toPay) + "<br>" + totalUpkeep.renderHtml(), gs);
    }
}

int GameEngine::getBuyPrice(Resources res, const Resources& market) const {
    int ret = 0;
    ret += res.amount[Money];

    for (size_t i = 2; i < res.amount.size(); i++) {
        int amntAtMarket = market.amount[i];
        int amnt = res.amount[i];
        while (amnt > 0) {
            ret += rules.pricePerAmount[amntAtMarket];
            if (amntAtMarket > 0) {
                amntAtMarket--;
            }
            amnt--;
        }
        if (amnt < 0) {
            ret -= getRules().marketSaleTax * amnt;
        }
        while (amnt < 0) {
            if (amntAtMarket + 1 < rules.pricePerAmount.size()) {
                ret -= rules.pricePerAmount[amntAtMarket + 1];
                amntAtMarket++;
            }
            amnt++;
        }
    }

    return ret;
}

int GameEngine::buyOrSellMats(Resources res, Resources& market) const {
        auto& mart = market.amount;
        int ret = 0;
        ret += res.amount[Money];

        for (int i = 2; i < res.amount.size(); i++) {
            int amnt = res.amount[i];
            while (amnt > 0) {
                ret += rules.pricePerAmount[mart[i]];
                if (mart[i] > 0) {
                    mart[i]--;
                }
                amnt--;
            }
            if (amnt < 0) {
                ret -= getRules().marketSaleTax * amnt;
            }
            while (amnt < 0) {
                if (mart[i] + 1 < rules.pricePerAmount.size()) {
                    ret -= rules.pricePerAmount[mart[i] + 1];
                    mart[i]++;
                }
                amnt++;
            }
        }

        return ret;
    }

void GameEngine::doAction(Action action, GameState& gs, bool withLogs, bool withStats) const {
        auto& ps = gs.players[gs.activePlayer];

        if (withStats) {
            auto& curStat = stats.pis.at(gs.activePlayer);
            if(!curStat.actionsLog.empty()) {
                curStat.actionAfterActionStat
                    .at(static_cast<int>(curStat.actionsLog.back().type))
                    .at(static_cast<int>(action.type))++;
            }
            curStat.actionsLog.push_back(action);
        }

        switch (action.type) {
            case ActionType::BuyWorker: {
                const auto& w = rules.workerCards.at(action.param);
                int price = buyOrSellMats(w.price, gs.market);
                ps.resources.amount[Money] -= price;
                for (const auto& [amnt, i] : enumerate(w.workerTypes)) {
                    ps.nWorkers[i] += amnt;
                }

                ps.updateUniqueWorkerTypes();

                break;
            }
            case ActionType::BuyWorkerForFree: {
                const auto& w = rules.workerCards.at(action.param);
                for (const auto& [amnt, i] : enumerate(w.workerTypes)) {
                    ps.nWorkers[i] += amnt;
                }

                ps.updateUniqueWorkerTypes();

                break;
            }
            case ActionType::Build: {
                const auto& bb = gs.basicBuildings.at(action.param).back();
                int price = buyOrSellMats(bb->price, gs.market);
                ps.resources.amount[Money] -= price;
                ps.buildings.emplace_back(bb);
                gs.basicBuildings.at(action.param).pop_back();

                ps.updateBuildingsByColor();
                ps.updateUniqueBuildingColors();

                break;
            }
            case ActionType::Produce: {
                const auto resBalance = bots[gs.activePlayer]->chooseWorkersToDeploy(*this, gs);
                
                if (resBalance.amount[WinPoints] > 0) {
                    awardWp(gs, resBalance.amount[WinPoints], ProductionWp, withStats);
                }

                if (getBuyPrice(resBalance * -1, gs.market) > 0) {
                    const auto temp_deployment = bots[gs.activePlayer]->chooseWorkersToDeploy(*this, gs);
                }

                // log(resBalance.renderHtml());
                int spentMoney = buyOrSellMats(resBalance * -1, gs.market);
                // int sold = 0;
                // for (const auto& r: resBalance.amount) {
                //     if (r > 0) {
                //         sold += r;
                //     }
                // }
                // spentMoney += rules.marketSaleTax * sold;
                ps.resources.amount[Money] -= spentMoney;

                // log("Profit: " + std::to_string(-spentMoney));

                // assert(ps.resources.amount[Money] >= 0);

                for (const auto& r: ps.resources.amount) {
                    assert(r >= 0);
                }

                break;
            }
            case ActionType::Borrow: {
                ps.resources.amount[Money] += rules.borrowValue;
                break;
            }
            case ActionType::DeployToMarket: {
                auto resBalance = bots[gs.activePlayer]->chooseResourcesForMarket(*this, gs, action.param);
                assert(resBalance.amount[WinPoints] >= 0);
                // assert(resBalance.amount[Money] == 0);

                if (resBalance.amount[WinPoints] > 0) {
                    awardWp(gs, resBalance.amount[WinPoints] * rules.marketWpCoef, MarketDeployWp, withStats);
                    resBalance.amount[WinPoints] = 0;
                }
                gs.market += resBalance;

                int money = abssum(resBalance.amount);
                ps.resources.amount[Money] += money * rules.marketWorkerMoneyBonus;

                // assert(ps.resources.amount[Money] >= 0);
                break;
            }
            case ActionType::UpgradeWorker: {
                const auto& g = getRules().workerUpgades.at(action.param);
                int gradePrice = getBuyPrice(g.price, gs.market);
                while (gradePrice <= ps.resources.amount[Money] && (ps.nWorkers.at(g.fromIdx) > 0)) {
                    ps.resources.amount[Money] -= buyOrSellMats(g.price, gs.market);
                    gradePrice = getBuyPrice(g.price, gs.market);
                    ps.nWorkers.at(g.fromIdx)--;
                    ps.nWorkers.at(g.toIdx)++;
                }
                break;
            }
            case ActionType::CreateQuarter: {
                assert(false);
                break;
            }
        }

        for (const auto& r: ps.resources.amount) {
            assert(r >= 0);
        }

        ps.updateQuestProgress(*this, gs);
    }

std::vector<Action> GameEngine::generateActions(const GameState& gs) const {
    std::vector<Action> ret;
    const auto& ps = gs.players[gs.activePlayer];
    // BuyWorker,
    for (const auto& [w, i]: enumerate(rules.workerCards)) {
        int price = getBuyPrice(w.price, gs.market);
        if (ps.resources.amount[Money] >= price) {
            ret.emplace_back(Action{ActionType::BuyWorker, (int) i});
        }
    }

    // Build,
    for (const auto& [b, i]: enumerate(gs.basicBuildings)) {
        if (!b.empty()) {
            int price = getBuyPrice(b.back()->price, gs.market);
            if (ps.resources.amount[Money] >= price) {
                ret.emplace_back(Action{ActionType::Build, (int) i});
            }
        }
    }

    // Produce, // + DeployWorker
    if (!ps.buildings.empty() && sum(ps.nWorkers) > 0) {
        ret.emplace_back(Action {ActionType::Produce, 0});
        // later we might add (.., 1) for different production strategies
    }

    if (sum(ps.nWorkers) > 0) {
        ret.emplace_back(Action {ActionType::DeployToMarket, 0});
        ret.emplace_back(Action {ActionType::DeployToMarket, 1});
        ret.emplace_back(Action {ActionType::DeployToMarket, 2});

        // later we might add (.., 1) for different deploy strategies
    }

    for (const auto& [g, idx]: enumerate(getRules().workerUpgades)) {
        if ((ps.nWorkers.at(g.fromIdx) > 0) && getBuyPrice(g.price, gs.market) <= ps.resources.amount[Money]) {
            ret.emplace_back(Action {ActionType::UpgradeWorker, (int) idx});
        }
    }

    // do we need it????
    ret.emplace_back(Action {ActionType::Borrow, rules.borrowValue});
    return ret;
}

std::string GameEngine::renderPsAsHtml(const GameState& gs) const {
    std::string ret;
    ret.reserve(1000);

    ret += "<details><summary>PS</summary><table border=1><tr><td>";
    const auto& ps = gs.players[gs.activePlayer];
    ret += "Money: " + std::to_string(ps.resources.amount[Money]) + "</td></tr><tr><td>";
    Resources totalUpkeep(rules.nResources);
    ret += OPEN_TABLE;
    for (const auto& [amnt, idx]: enumerate(ps.nWorkers)) {
        totalUpkeep += rules.workerUpkeepCost.at(idx) * amnt;

        if (amnt > 0) {
            ret += std::to_string(amnt) + "<br>" + workerPic(idx) + "</td><td>";
        }
    }
    ret += CLOSE_TABLE;

    ret += TABLE_NEW_LINE;
    for (const auto& b: ps.buildings) {
        ret += b->htmlTag + TABLE_NEW_LINE;
        totalUpkeep += b->upkeep;
    }
    ret += TABLE_NEW_LINE;
    ret += "Total upkeep price: " + std::to_string(getBuyPrice(totalUpkeep, gs.market)) + TABLE_NEW_LINE;
    ret += totalUpkeep.renderHtml() + TABLE_NEW_LINE;
    ret += "</td></tr></table></details>";

    return ret;
}

bool GameEngine::shouldPayUpkeep(const GameState& gs) const {
    // return false;
    return (gs.currentRound + 1) % rules.stepsBetweenUpkeep == 0;
}

std::string GameEngine::renderLogAsHtml() const {
    std::string ret;
    ret.reserve(10000);
    ret += "<html><body>";
    ret += "<table border=1><tr><td>";
    ret += getRules().renderRulesHtml();
    
    ret += TABLE_NEW_LINE;

    for (int round = 0; round < rules.totalRounds; round++) {
        for (const auto& playerLog: gameLog) {
            ret += OPEN_TABLE;
            for (const auto& s: playerLog[round]) {
                ret += s + TABLE_NEW_LINE;
            }
            ret += CLOSE_TABLE;
            ret += "</td><td>";
        }

        ret += TABLE_NEW_LINE;
    }

    ret += CLOSE_TABLE + "</body></html>";

    return ret;
}

bool GameEngine::gameEnded(const GameState& gs) const {
    return gs.currentRound >= rules.totalRounds;
}

void GameEngine::awardWp(GameState& gs, int amount, WpSource source, bool withStats) const {
    gs.players.at(gs.activePlayer).wpSources.at(source) += amount;
    gs.players.at(gs.activePlayer).resources.amount.at(WinPoints) += amount;
}

void GameEngine::resetLogsAndStats(int nPlayers) {
    gameLog.resize(nPlayers, std::vector<std::vector<std::string>>());
    stats = GameStats(nPlayers);
}

GameStats GameEngine::getStatsCopy() const {
    return stats;
}
