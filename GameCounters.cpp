#include "GameCounters.h"

#include "RandomBot.h"

GameCounters::GameCounters(const GameEngine& ge, const GameState gs) 
    : gameEnded(ge.gameEnded(gs))
    , productionResBalance(ge.getRules().nResources)
    , curRound(gs.currentRound)
    , gameStage(gs.currentRound / ge.getRules().totalRounds + 1e-3)
    , nWorkers(gs.getPS().nWorkers)
{
    static const auto botForWorkers = new RandomBot(0);

    const auto& ps = gs.getPS();

    productionResBalance = botForWorkers->chooseWorkersToDeploy(ge, gs, true);

    auto mart = gs.market;
    productionProfit = -ge.buyOrSellMats(productionResBalance * -1, mart);
    // assert(productionProfit >= 0);
    // int sold = 0;
    // for (const auto& r: productionResBalance.amount) {
    //     if (r > 0) {
    //         sold += r;
    //     }
    // }
    // productionProfit -= ge.getRules().marketSaleTax * sold;
    
    workerWp = 0;
    for (const auto& [amnt, idx]: enumerate(ps.nWorkers)) {
        workerWp += ge.getRules().workers.at(idx).winPoints * amnt;
    }
    buildingWp = 0;
    for (const auto& b: ps.buildings) {
        buildingWp += b->winPoints;
        buildings.emplace_back(b->idx);
    }
    for (const auto& b: gs.basicBuildings) {
        if (!b.empty()) {
            buildingsOnSale.push_back(b.front()->idx);
        }
    }

    for (auto& q: questProgress) {
        q = 0.0;
    }
    for (int qidx = 0; qidx < ps.questProgress.size(); qidx++) {
        questProgress.at(gs.questProgress.at(qidx).first.idx) = ps.questProgress.at(qidx);
    }

    pureWp = ps.resources.amount[WinPoints];
    moneyWp = ps.resources.amount[Money] / ge.getRules().moneyPerWp;

    // wpBySource
    for (int source = 0; source < WpSourcesTotal; source++) {
        wpBySource.at(source) = ps.wpSources.at(source);
    }
}

// std::vector<double> GameCounters::toVector() const {
//     assert(false);
//     std::vector<double> ret;
//     ret.reserve(40);

//     ret.push_back(gameEnded);

//     ret.push_back(productionResBalance.amount.size());
//     std::copy(productionResBalance.amount.begin(), productionResBalance.amount.end(), std::back_inserter(ret));
    
//     ret.push_back(curRound);
//     ret.push_back(gameStage);

//     ret.push_back(productionProfit);

//     ret.push_back(questProgress.size());
//     std::copy(questProgress.begin(), questProgress.end(), std::back_inserter(ret));
    
//     ret.push_back(workerWp);
//     ret.push_back(buildingWp);
//     ret.push_back(pureWp);
//     ret.push_back(moneyWp);

//     return ret;
// }

// GameCounters::GameCounters(const std::vector<double>& vec) {
//     auto iter = vec.begin();
//     gameEnded = *(iter++);

//     size_t sz = *(iter++);
//     std::copy(iter, iter + sz, std::back_inserter(productionResBalance.amount));
    
//     iter += sz;
//     curRound = *(iter++);
//     gameStage = *(iter++);
//     productionProfit = *(iter++);
    
//     sz = *(iter++);
//     assert(false);
//     // std::copy(iter, iter + sz, std::back_inserter(questProgress));
    
//     workerWp = *(iter++);
//     buildingWp = *(iter++);
//     pureWp = *(iter++);
//     moneyWp = *(iter++);
// }
