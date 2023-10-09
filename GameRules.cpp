#include "GameRules.h"

#include <algorithm>
#include <random>
#include <assert.h>

namespace {
    int getBuyPrice(const GameRules& rules, Resources res, const Resources& market) {
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
                ret -= rules.marketSaleTax * amnt;
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
}

GameRules::GameRules(int seed_) 
    : seed(seed_)
{
    // static std::random_device r;
    std::default_random_engine rng(seed);

    const int reservedResources = 2;

    totalUpkeeps = 4;
    stepsBetweenUpkeep = 6 + rng() % 7;
    totalRounds = totalUpkeeps * stepsBetweenUpkeep;

    int nWorkerResources = 2 + rng() % 3;
    int nBuildingResources = 4 + rng() % 7;
    int resourceOverlap = std::min(nWorkerResources - 1, (int) (rng() % 3));
    nResources = reservedResources + nWorkerResources + nBuildingResources - resourceOverlap;

    Resources genericResourceValue(nResources);
    moneyPerWp = 10;
    // moneyPerWp = rng() % 10 + 3;
    genericResourceValue.amount[WinPoints] = moneyPerWp;
    genericResourceValue.amount[Money] = 1;
    for (int i = reservedResources; i < nResources; i++) {
        genericResourceValue.amount[i] = 1 + rng() % 12;
    }
    // int maxResourceValue = *std::max_element(genericResourceValue.amount.begin(), genericResourceValue.amount.end());
    
    workers.clear();
    workers.push_back(Worker{
        .marketPower = 1,
        .productionPower = 1,
        .winPoints = 0,
        .price = Resources{},
        .color = (int) workers.size(),
        .canGoToOpp = false,
    });
    workers.push_back(Worker{
        .marketPower = 2 + (int) rng() % 3,
        .productionPower = 1,
        .winPoints = 0,
        .price = Resources{},
        .color = (int) workers.size(),
        .canGoToOpp = false,
    });
    workers.push_back(Worker{
        .marketPower = 1,
        .productionPower = 2 + (int) rng() % 3,
        .winPoints = 0,
        .price = Resources{},
        .color = (int) workers.size(),
        .canGoToOpp = false,
    });
    workers.push_back(Worker{
        .marketPower = 1,
        .productionPower = 1,
        .winPoints = 3 + (int) rng() % 10,
        .price = Resources{},
        .color = (int) workers.size(),
        .canGoToOpp = false,
    });
    nWorkerGrades = workers.size();

    // haveWorkerUpkeep = rng() % 2;
    haveWorkerUpkeep = 0;
    if (haveWorkerUpkeep) {
        for (int i = 0; i < nWorkerGrades; i++) {
            workerUpkeepCost.emplace_back(nResources, reservedResources, reservedResources + nWorkerResources, (i + rng() % 3));
        }
    } else {
        workerUpkeepCost.resize(nWorkerGrades, Resources(nResources));
    }

    workerUpgades.push_back(WorkerUpgrade{
        .fromIdx = 0,
        .toIdx = 1,
        .price = Resources(nResources, reservedResources, reservedResources + nWorkerResources, 0, 2),
    });
    workerUpgades.push_back(WorkerUpgrade{
        .fromIdx = 0,
        .toIdx = 2,
        .price = Resources(nResources, reservedResources, reservedResources + nWorkerResources, 0, 2),
    });
    workerUpgades.push_back(WorkerUpgrade{
        .fromIdx = 0,
        .toIdx = 3,
        .price = Resources(nResources, reservedResources, reservedResources + nWorkerResources, 0, 2),
    });

    for (const auto& w: workers) {
        workersByProductionPower.emplace_back(&w);
    }
    std::sort(workersByProductionPower.begin(), workersByProductionPower.end(),
        [](const Worker* op1, const Worker* op2) {
            return op1->productionPower > op2->productionPower;
    });

    int nWorkerCards = 2 + rng() % 4;
    for (int i = 0; i < nWorkerCards; i++) {
        std::vector<int> workerTypes(nWorkerGrades, 0);
        workerTypes[0] = 1 + i * (i + 1) / 2;

        Resources cardPrice(nResources);
        for (int w = 0; w < nWorkerGrades; w++) {
            cardPrice += workerUpkeepCost[w] * workerTypes[w];
        }
        cardPrice += Resources(nResources, reservedResources, reservedResources + nWorkerResources, 0, 2 + i);
        if (!workerCards.empty()) {
            cardPrice += workerCards.back().price;
        }
        std::string htmlTag = "<table border=1><tr><td>" + cardPrice.renderHtml() + "</td></tr><tr><td>";
        for (const auto& [amnt, idx] : enumerate(workerTypes)) {
            for (int j = 0; j < amnt; j++) {
                htmlTag += workerPic(idx);
            }
        }
        htmlTag += "</td></tr></table>";

        workerCards.push_back(WorkerCard{
            cardPrice,
            workerTypes,
            std::move(htmlTag)
        });
    }

    // int nThresholds = maxResourceValue + rng() % 3;
    int nThresholds = 8;
    thresholdType = rng() % 2;

    // int inc = 1;
    int groupSize = 2 + rng() % 5;
    marketMaxResources = groupSize * nThresholds;
    priceThresholds = {
        {0, marketMaxResources},
        {1, marketMaxResources - groupSize * 1},
        {2, marketMaxResources - groupSize * 2},
        {3, marketMaxResources - groupSize * 3},
        {4, marketMaxResources - groupSize * 4},
        {5, marketMaxResources - groupSize * 5},
        {7, marketMaxResources - groupSize * 6},
        {10, marketMaxResources - groupSize * 7},
    };
    // priceThresholds.emplace_back(0, marketMaxResources);
    // for (int i = 1; i < nThresholds; i++) {
    //     inc += rng() % 2;
    //     priceThresholds.emplace_back(
    //         priceThresholds.back().first + inc,
    //         priceThresholds.back().second - groupSize
    //     );
    // }
    assert(priceThresholds.back().second == groupSize);

    pricePerAmount.push_back(priceThresholds.back().first);
    for (const auto& [price, amnt]: reversed(priceThresholds)) {
        for (int j = 0; j < groupSize; j++) {
            pricePerAmount.push_back(price);
        }
        assert(pricePerAmount.size() == amnt + 1);
    }
    assert(pricePerAmount.back() == 0);

    marketStartingResources = Resources(nResources);
    for (int i = reservedResources; i < nResources; i++) {
        marketStartingResources.amount[i] = groupSize * (
            std::max(0, (int) priceThresholds.size() - genericResourceValue.amount[i])
        );
    }
    marketSaleTax = rng() % 3;
    marketWorkerMoneyBonus = rng() % 3;
    maxWorkersAtMarket = 4 + rng() % 5;

    std::sort(workerCards.begin(), workerCards.end(), [&] (const WorkerCard& op1, const WorkerCard& op2) {
        return genericResourceValue.dot(op1.price) <= genericResourceValue.dot(op2.price);
    });

    nBuildingGroups = 4 + rng() % 2;
    int foodBuildings = 3 + rng() % 5;
    int productionBuildings = 6 + rng() % 10;
    int winpointsBuildings = 5 + rng() % 8;
    int effectBuildings = 3 + rng() % 5;
    nBuildingColors = 4;
    
    int nBuildings = foodBuildings + productionBuildings + winpointsBuildings + effectBuildings;
    
    int buildStartResources = reservedResources + nWorkerResources - resourceOverlap;
    int workerEndResources = reservedResources + nWorkerResources;
    // haveBuildingUpkeep = rng() % 2;
    haveBuildingUpkeep = 0;
    
    size_t buildingIdx = 0;
    for (int i = 0; i < foodBuildings; i++) {
        Resources upkeep(nResources, workerEndResources, nResources, 1);
        if (!haveBuildingUpkeep) {
            upkeep = Resources(nResources);
        }
        int upkeepValue = genericResourceValue.dot(upkeep);

        Resources input(nResources, workerEndResources, nResources, 0 + rng() % 3);
        int inputValue = genericResourceValue.dot(input);
        Resources output(nResources, reservedResources, workerEndResources, input, 1 + rng() % 3);
        int outputValue = genericResourceValue.dot(output);

        int retry = 0;
        while (outputValue <= inputValue + upkeepValue) {
            output = Resources(nResources, reservedResources, workerEndResources, input, 1 + retry / 10 + rng() % 3);
            outputValue = genericResourceValue.dot(output);
            retry++;
        }
        int profitValue = outputValue - inputValue - upkeepValue / 2;
        // int workerColor = profitValue / 2 - 1 + rng() % 3;
        // workerColor = std::max(0, std::min(workerColor, nWorkerGrades - 1));
        int workerColor = 0;
        int maxWorkers = 1 + rng() % 2;
        Resources price(nResources, buildStartResources, nResources, 1 + profitValue / 2 + maxWorkers / 2);
        int priceValue = genericResourceValue.dot(price);
        int diff = std::max(0ul, 3 * (-outputValue + inputValue + upkeepValue) + priceValue + rng() % 8);
        int winPoints = std::max(0, diff / genericResourceValue.amount[WinPoints]);

        double priority = priceValue * 30 + inputValue * 5 + outputValue;

        std::string htmlTag = "<table border=1><tr><td>" + price.renderHtml() + "</td><td>" + std::to_string(priceValue) + "</td></tr><tr><td>" +
            upkeep.renderHtml() + "</td><td>" + std::to_string(upkeepValue) + "</td></tr><tr><td>" +
            input.renderHtml() + "</td><td><center>" + std::to_string(maxWorkers) + "</center><br>" + workerPic(workerColor) + "</td><td>" + output.renderHtml() + "</td></tr><tr><td colspan=\"3\">" +
            "<center>WP: " + std::to_string(winPoints) + " prio: " + std::to_string(priority) + " production: " + std::to_string(profitValue) + "</center>" +
            "</td></tr></table>";

        basicBuildings.push_back(Building{
            winPoints, input, output, upkeep, price, workerColor, maxWorkers, priority, htmlTag, (int) rng() % nBuildingColors, buildingIdx
        });
        buildingIdx++;
    }

    while (basicBuildings.size() < nBuildings) {
        Resources upkeep(nResources, buildStartResources, nResources, 1);
        if (!haveBuildingUpkeep) {
            upkeep = Resources(nResources);
        }
        int upkeepValue = genericResourceValue.dot(upkeep);
        int wupkeepValue = genericResourceValue.dot(workerUpkeepCost.back()) / 2;

        Resources input(nResources, buildStartResources, nResources, 1 + rng() % 2);
        int inputValue = genericResourceValue.dot(input);
        Resources output(nResources, reservedResources, nResources, input, 1 + rng() % 3);
        int outputValue = genericResourceValue.dot(output);

        int retry = 0;
        while (outputValue <= inputValue + upkeepValue + wupkeepValue) {
            output = Resources(nResources, reservedResources, nResources, input, 1 + retry / 10 + rng() % 3);
            output.amount[WinPoints] = std::max(0, (int) (rng() % 10) - 6);
            outputValue = genericResourceValue.dot(output);
            retry++;
        }
        int profitValue = outputValue - inputValue - upkeepValue / 2;
        int workerColor = profitValue / 3 - 1 + rng() % 3;
        workerColor = std::max(0, std::min(workerColor, nWorkerGrades - 1));
        int maxWorkers = 1 + rng() % 5;
        Resources price(nResources, buildStartResources, nResources, 1 + profitValue / 2 + maxWorkers / 2);
        int priceValue = genericResourceValue.dot(price);
        int diff = std::max(0ul, 3 * (-outputValue + inputValue + upkeepValue) + priceValue + rng() % 8);
        int winPoints = std::max(0, diff / genericResourceValue.amount[WinPoints]);

        double priority = priceValue * 30 + inputValue * 5 + outputValue;

        std::string htmlTag = "<table border=1><tr><td>" + price.renderHtml() + "</td><td>" + std::to_string(priceValue) + "</td></tr><tr><td>" +
            upkeep.renderHtml() + "</td><td>" + std::to_string(upkeepValue) + "</td></tr><tr><td>" +
            input.renderHtml() + "</td><td><center>" + std::to_string(maxWorkers) + "</center><br>" + workerPic(workerColor) + "</td><td>" + output.renderHtml() + "</td></tr><tr><td colspan=\"3\">" +
            "<center>WP: " + std::to_string(winPoints) + " prio: " + std::to_string(priority) + " production: " + std::to_string(profitValue) + "</center>" +
            "</td></tr></table>";

        basicBuildings.push_back(Building{
            winPoints, input, output, upkeep, price, workerColor, maxWorkers, priority, htmlTag, (int) rng() % nBuildingColors, buildingIdx
        });
        buildingIdx++;
    }

    std::sort(basicBuildings.begin(), basicBuildings.end(), 
        [](const Building& op1, const Building& op2) { return op1.priority <= op2.priority; });

    for (int i = 0; i < basicBuildings.size(); i++) {
        basicBuildings[i].priority = i;
    }
    // std::vector<Building> advancedBuildings;

    playerStartingResources = Resources(nResources);
    playerStartingResources.amount[Money] = getBuyPrice(*this, workerCards.at(rng() % nWorkerCards).price, marketStartingResources) +
        rng() % 20;
    const auto startingGoldB = 2 + rng() % 3;
    for (int bIdx = 0; bIdx < startingGoldB; bIdx++) {
        playerStartingResources.amount[Money] += getBuyPrice(*this, basicBuildings.at(bIdx).price, marketStartingResources);
    }

    borrowValue = (playerStartingResources.amount[Money] + rng() % (playerStartingResources.amount[Money])) / 30;

    maxPlayersPerQuest = 1 + rng() % 3;
    wpDiscountForQuest = {1.0, urand(rng, 0.5, 0.9), urand(rng, 0.2, 0.5), 0.0};
    int nQuests = 30 + rng() % 30;
    for (int i = 0; i < nQuests; i++) {
        std::vector<QuestRequirement> reqs;
        int nReqs = 1 + rng() % 3;
        int winPoints = (5 + lnrand(rng) * 4) * nReqs;
        for (int r = 0; r < nReqs; r++) {
            QuestRequirementType type = static_cast<QuestRequirementType>(rng() % 7);
            int paramType = -123;
            int paramAmount = -123;
            switch (type) {
                case QuestRequirementType::GoldAmount:
                    paramAmount = playerStartingResources.amount[Money] + lnrand(rng) * 100;
                    break;
                case QuestRequirementType::ExactBuildingColorAmount:
                    paramAmount = 2 + lnrand(rng) * 3;
                    paramType = rng() % nBuildingColors;
                    break;
                case QuestRequirementType::TotalBuildingsAmount:
                    paramAmount = 3 + lnrand(rng) * 7;
                    break;
                case QuestRequirementType::UniqueBuildingColors:
                    paramAmount = 1 + rng() % nBuildingColors;
                    break;
                case QuestRequirementType::TotalWorkersAmount:
                    paramAmount = 6 + lnrand(rng) * 6;
                    break;
                case QuestRequirementType::UniqueWorkerTypes:
                    // paramAmount = 3 + rng() % (nWorkerGrades - 2);
                    r--;
                    continue;
                    break;
                case QuestRequirementType::ExactWorkerTypeAmount:
                    paramAmount = 3 + lnrand(rng) * 3;
                    paramType = rng() % nWorkerGrades;
                    break;
            }
            assert(paramAmount >= 0);
            reqs.emplace_back(QuestRequirement{type, paramType, paramAmount});
        }
        quests.emplace_back(Quest{reqs, winPoints, i});
    }
    nQuestsInMatch = 6 + rng() % 10;
}

std::string GameRules::renderRulesHtml() const {
    std::string ret;
    ret.reserve(100000);

    ret += "<details><table border=1><tr><td><table><tr><td>";
    ret += "tuneVersion: " + std::to_string(tuneVersion) + "</td></tr><tr><td>";
    ret += "seed: " + std::to_string(seed) + "</td></tr><tr><td>";
    ret += "nResources: " + std::to_string(nResources) + "</td></tr><tr><td>";
    ret += "totalUpkeeps: " + std::to_string(totalUpkeeps) + "</td></tr><tr><td>";
    ret += "stepsBetweenUpkeep: " + std::to_string(stepsBetweenUpkeep) + "</td></tr><tr><td>";
    ret += "totalRounds: " + std::to_string(totalRounds) + "</td></tr><tr><td>";
    ret += "playerStartingResources: " + playerStartingResources.renderHtml() + "</td></tr><tr><td>";

    ret += "</td></tr></table>WORKERS</td></tr><tr><td><table><tr><td>";

    for (const auto& [res, idx]: enumerate(workerUpkeepCost)) {
        ret += workerPic(idx) + "</td><td>" + res.renderHtml() + "</td><td>wp: " + std::to_string(workers.at(idx).winPoints) + "</td></tr><tr><td>";
    }

    for (const auto& wc: workerCards) {
        ret += wc.htmlTag + "</td></tr><tr><td>";
    }

    ret += "</td></tr></table>MARKET</td></tr><tr><td><table><tr><td>";

    ret += "<table border=1><tr><td>price</td><td>amnt</td></tr><tr><td>";
    for (const auto& [price, th]: priceThresholds) {
        ret +=  std::to_string(price) + "</td><td>" + std::to_string(th) + "</td></tr><tr><td>";
    }
    ret += "</td></tr></table></td></tr><tr><td>";
    
    ret += marketStartingResources.renderHtml() + "</td></tr><tr><td>";
    ret += "marketSaleTax: " + std::to_string(marketSaleTax) + "</td></tr><tr><td>";
    ret += "marketWpCoef: " + std::to_string(marketWpCoef) + "</td></tr><tr><td>";
    
    ret += "</td></tr></table>BUILDINGS</td></tr><tr><td><table><tr><td>";

    ret += "nBuildingGroups: " + std::to_string(nBuildingGroups) + "</td></tr><tr><td>";

    for (const auto& b: basicBuildings) {
        ret += "starting price: " + std::to_string(getBuyPrice(*this, b.price, marketStartingResources)) + "<br>" + b.htmlTag + "</td></tr><tr><td>";
    }

    ret += "</td></tr></table> </td></tr></table></details>";

    return ret;
}
