#include "IBot.h"

#include "GameEngine.h"
#include "Utils.h"
#include "GameCounters.h"
#include "Types.h"

#include <array>
#include <unordered_map>

Resources IBot::chooseWorkersToDeploy(const GameEngine& ge, const GameState& gs, bool fast) const {
    const auto& ps = gs.players[gs.activePlayer];
    Resources ret(ge.getRules().nResources);

    if (ps.buildings.empty() || sum(ps.nWorkers) == 0) {
        return ret;
    }

    // auto spareWorkers = ps.nWorkers;
    std::vector<int> spareWorkers;
    spareWorkers.reserve(100);
    for (const auto& wIdx: reversed(ge.getRules().workersByProductionPower)) {
        for (int i = 0; i < ps.nWorkers.at(wIdx->color); i++) {
            spareWorkers.emplace_back(wIdx->color);
        }
    }
    auto mart = gs.market;
    std::vector<std::pair<int, int>> profits;

    if (!fast) {
        std::unordered_map<int, int> buildingSlots;
        buildingSlots.reserve(ps.buildings.size() * 2);

        for (int i = 0; i < ps.buildings.size(); ++i) {
            buildingSlots.emplace(i, ps.buildings[i]->maxWorkers);
        }

        while(!buildingSlots.empty() && !spareWorkers.empty()) {
            int bestProfit = 0;
            int bestIdx = -1;
            for (const auto& [idx, slots]: buildingSlots) {
                const auto& b = ps.buildings[idx];

                int profit = -ge.getBuyPrice(b->in - b->out, mart);
                if (profit > bestProfit) {
                    bestProfit = profit;
                    bestIdx = idx;
                }
            }

            if (bestProfit <= 0) {
                break;
            }

            while (bestProfit > 0 && !buildingSlots.empty() && !spareWorkers.empty()) {
                const auto& worker = ge.getRules().workers.at(spareWorkers.back());
                auto productionPower = worker.productionPower;
                buildingSlots.at(bestIdx)--;
                spareWorkers.pop_back();
                if (buildingSlots.at(bestIdx) == 0) {
                    buildingSlots.erase(bestIdx);
                }
                while (bestProfit > 0 && productionPower > 0) {
                    ret += ps.buildings[bestIdx]->out;
                    ret -= ps.buildings[bestIdx]->in;
                    mart += ps.buildings[bestIdx]->out;
                    mart -= ps.buildings[bestIdx]->in;
                    mart.clip(0, ge.getRules().marketMaxResources);
                    bestProfit = -ge.getBuyPrice(ps.buildings[bestIdx]->in - ps.buildings[bestIdx]->out, mart);
                    productionPower--;
                }

                if (buildingSlots.count(bestIdx) == 0) {
                    break;
                }
            }
        }
    } else {
        for (const auto& [b, idx]: enumerate(ps.buildings)) {
            profits.emplace_back(-ge.getBuyPrice(b->in - b->out, mart), idx);
        }

        std::sort(profits.begin(), profits.end());
        std::reverse(profits.begin(), profits.end());

        for (const auto& [proft, idx]: profits) {
            if (proft <= 0 || spareWorkers.empty()) {
                break;
            }
            const auto& b = ps.buildings[idx];

            int amnt = std::min((int) spareWorkers.size(), b->maxWorkers);

            for (int j = 0; j < amnt; j++) {
                const auto curProfit = -ge.getBuyPrice(b->in - b->out, mart);
                const auto martCopy(mart);
                if (curProfit <= 0) {
                    break;
                }
                const auto& worker = ge.getRules().workers.at(spareWorkers.back());
                mart += b->out * worker.productionPower;
                mart -= b->in * worker.productionPower;
                mart.clip(0, ge.getRules().marketMaxResources);
                spareWorkers.pop_back();
                ret += b->out * worker.productionPower;
                ret -= b->in * worker.productionPower;
            }
        }
    }
    return ret;
}

Resources IBot::chooseResourcesForMarket(const GameEngine& ge, const GameState& gs, int param) const {
    const auto& ps = gs.players[gs.activePlayer];
    Resources ret(ge.getRules().nResources);

    int resourcesToMove = 0;
    for (int grade = 0; grade < ps.nWorkers.size(); grade++) {
        resourcesToMove += ge.getRules().workers.at(grade).marketPower * ps.nWorkers.at(grade);
    }
    resourcesToMove = std::min(ge.getRules().maxWorkersAtMarket, resourcesToMove);

    if (param == 2) {
        ret.amount[WinPoints] = resourcesToMove;
        return ret;
    }
    if (param == 1) {
        ret.amount[Money] = resourcesToMove;
        return ret;
    }

    Resources productionBalance(ge.getRules().nResources);
    for (const auto& b: ps.buildings) {
        productionBalance += b->out;
        productionBalance -= b->in;
    }
    Resources upkeepBalance(ge.getRules().nResources);
    for (const auto& b: ps.buildings) {
        upkeepBalance += b->upkeep;
    }
    for (const auto& [w, idx]: enumerate(ps.nWorkers)) {
        upkeepBalance += ge.getRules().workerUpkeepCost.at(idx) * w;
    }
    productionBalance = productionBalance * 2 - upkeepBalance;

    // assert(resourcesToMove < 20);

    if (abssum(productionBalance.amount) == 0) {
        for (const auto& b: gs.basicBuildings) {
            if (!b.empty()) productionBalance -= b.back()->price;
        }
        for (const auto& wc: ge.getRules().workerCards) {
            productionBalance -= wc.price;
        }
    }

    const auto marketPrices = argsort(gs.market.amount);
    for (const auto& productId: marketPrices) {
        // if (productId < 2) {
        //     // Can't move money and winpoints are already moved
        //     continue;
        // }
        if (productionBalance.amount.at(productId) > 0) {
            int workers = std::min(resourcesToMove, gs.market.amount.at(productId));
            ret.amount.at(productId) -= workers;
            resourcesToMove -= workers;
        } else if (productionBalance.amount.at(productId) < 0) {
            int workers = std::min(resourcesToMove, ge.getRules().marketMaxResources - gs.market.amount.at(productId));
            ret.amount.at(productId) += workers;
            resourcesToMove -= workers;
        }
        if (resourcesToMove <= 0) {
            break;
        }
    }
    // assert(resourcesToMove == 0);

    return ret;
}

size_t IBot::chooseStartingWorkerCard(const GameEngine& ge, const GameState& gs, int param) const {
    return ge.getRules().workerCards.size() - 1;
}

double IBot::measureGs(const GameCounters& gc) const {
    return ::measureGs(gc, measurerParams);
}

double measureGs(const std::array<double, WpSourcesTotal>& sourceWeights, const GameCounters& gc, const MeasurerParams& params) {
    if (gc.gameEnded) {
        double wps = 0.0;
        for (int source = 0; source < WpSourcesTotal; source++) {
            wps += gc.wpBySource.at(source) * sourceWeights.at(source);
        }
        // assert(wps >= 0);
        return wps;
    }
    return measureGs(gc, params);
}


double measureGs(const GameCounters& gc, const MeasurerParams& params) {
    if (gc.gameEnded) {
        assert(gc.pureWp >= 0);
        return gc.pureWp;
    }

    double questPoints = 0.0;
    for (size_t qidx = 0; qidx < gc.questProgress.size(); qidx++) {
    // for (const auto& [p, qidx]: enumerate()) {
        questPoints += gc.questProgress.at(qidx) * params.questCoef.at(qidx);
    }

    double buildingPoints = 0.0;
    for (const auto& b: gc.buildings) {
        buildingPoints += params.buildingCoef.at(b);
    }
    double buildingOnSalePoints = 0.0;
    for (const auto& b: gc.buildingsOnSale) {
        buildingOnSalePoints += params.buildingCoef.at(b);
    }

    double resourceIncomePoints = 0.0;
    for (size_t ridx = 0; ridx < gc.productionResBalance.amount.size(); ridx++) {
        resourceIncomePoints += gc.productionResBalance.amount.at(ridx) * params.resourceIncomeCoef.at(ridx);
    }

    double workersPoints = 0.0;
    for (const auto& [w, idx]: enumerate(gc.nWorkers)) {
        workersPoints += w * params.workerCoef.at(idx);
    }

    double pts = gc.pureWp + 
        // pow(gameStage, params.incomePow) * params.incomeCoef * productionProfit +
        params.incomeCoef * gc.productionProfit +
        std::max(0, gc.moneyWp) * params.moneyCoef +
        questPoints +
        gc.buildingWp * params.buildingWpCoef +
        gc.workerWp * params.workerWpCoef + 
        buildingPoints +
        buildingOnSalePoints * params.buildingOnSaleCoef +
        resourceIncomePoints +
        workersPoints;
    assert(std::isfinite(pts));
    return pts;
}
