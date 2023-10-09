#include "MctsBot.h"

#include "GameEngine.h"
#include "Timer.h"

#include <assert.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <memory>

struct MctsNode {
    const MctsNode* findBestChild() const {
        assert (!children.empty());

        const MctsNode *bestChild = &children.front();
        double bestPts  = bestChild->aggPts;
        for (const auto& child: children) {
            if (child.aggPts > bestPts) {
                bestPts = child.aggPts;
                bestChild = &child;
            }
        }

        return bestChild;
    }

    // std::unique_ptr<GameState> gsPtr;
    Action actionToGetHere = Action{};
    MctsNode* parent = nullptr;
    int depth = 0;

    double pts = -123;
    double aggPts = -123;
    bool generatedChildren = false;
    std::vector<MctsNode> children;
    int stepIns = 0;
    bool nodeIsCompletelyEvaluated = false;
};

void sumAndValidate(const MctsNode& node) {
    assert(std::isfinite(node.pts));
    assert(std::isfinite(node.aggPts));

    if (!node.generatedChildren) {
        return;
    }
    int sum = 0;
    for (const auto& child: node.children) {
        sumAndValidate(child);
        sum += child.stepIns;
    }
    assert((sum == node.stepIns) || node.nodeIsCompletelyEvaluated);
}

void MctsBot::advanceToNextNode(const Action& action, GameState& gs, const GameEngine& ge) const {
    ge.doAction(action, gs);
    const auto myPlayer = gs.activePlayer;
    ge.doAfterTurnActions(gs);
    while (myPlayer != gs.activePlayer) {
        if (oppBot == nullptr) {
            ge.doAction(Action{ActionType::Borrow, 0}, gs);
        } else {
            ge.doAction(oppBot->chooseAction(ge, gs), gs);
        }
        ge.doAfterTurnActions(gs);
    }

    if (ge.gameEnded(gs)) {
        ge.scoreFinalPoints(gs, false);
    }
}

void MctsBot::genChildren(MctsNode& node, const GameState& gs, const GameEngine& ge) const {        
    const auto actions = ge.generateActions(gs);
    node.children.reserve(actions.size());
    for (const auto& action: actions) {
        GameState newState(gs);
        advanceToNextNode(action, newState, ge);

        double pts = measureGs(GameCounters(ge, newState));
        node.children.emplace_back(MctsNode{
            .actionToGetHere = action,
            .parent = &node,
            .depth = node.depth + 1,
            .pts = pts,
    // double aggPts = -123;
    // bool generatedChildren = false;
    // std::vector<MctsNode> children;
    // int stepIns = 0;
    // bool nodeIsCompletelyEvaluated = false;
        });
        if (ge.gameEnded(newState)) {
            node.children.back().nodeIsCompletelyEvaluated = true;
            node.children.back().generatedChildren = true;
            node.children.back().aggPts = pts;
        }
    }

    node.generatedChildren = true;
}

MctsNode* MctsBot::goBottom(MctsNode& node, const GameState& gs, const GameEngine& ge) const {
    MctsNode* curNode = &node;
    GameState curGs(gs);

    while (curNode != nullptr) {
        curNode->stepIns++;
        if ((curNode->depth >= params.maxDepth) || ge.gameEnded(curGs) || curNode->nodeIsCompletelyEvaluated) {
            break;
        }

        if (!curNode->generatedChildren) {
            genChildren(*curNode, curGs, ge);
        }

        // Need exploration?
        std::vector<MctsNode*> explorationCandidates;
        explorationCandidates.reserve(curNode->children.size());
        for (auto& child: curNode->children) {
            if (child.stepIns == 0) {
                explorationCandidates.push_back(&child);
            }
        }
        if (!explorationCandidates.empty()) {
            double bestPts  =-1e9;
            MctsNode *bestChild = explorationCandidates.front();
            for (auto& child: explorationCandidates) {
                if (child->pts > bestPts) {
                    bestPts = child->pts;
                    bestChild = child;
                }
            }

            curNode = bestChild;
            advanceToNextNode(curNode->actionToGetHere, curGs, ge);
        } else {
            double bestPts  =-1e9;
            assert(!curNode->children.empty());
            MctsNode *bestChild = &(curNode->children.front());
            double minAgg = curNode->children.front().aggPts;
            double maxAgg = curNode->children.front().aggPts;
            double minPts = curNode->children.front().pts;
            double maxPts = curNode->children.front().pts;
            for (auto& child: curNode->children) {
                minAgg = std::min(child.aggPts, minAgg);
                maxAgg = std::max(child.aggPts, maxAgg);

                minPts = std::min(child.pts, minPts);
                maxPts = std::max(child.pts, maxPts);
            }
            double deltaAgg = maxAgg - minAgg + 1e-6;
            double deltaPts = maxPts - minPts + 1e-6;

            double gameStage = (curGs.currentRound + 2.0) / (ge.getRules().totalRounds + 1.0);
            assert(gameStage <= 1.0);

            for (auto& child: curNode->children) {
                if (child.nodeIsCompletelyEvaluated) {
                    continue;
                }

                double mctsPoints = gameStage * (child.aggPts - minAgg) / deltaAgg
                    + (1.0 - gameStage) * (child.pts - minPts) / deltaPts
                    + params.C * sqrt(log(curNode->stepIns) / (child.stepIns + 1e-6));

                if (mctsPoints > bestPts) {
                    bestPts = mctsPoints;
                    bestChild = &child;
                }
            }

            curNode = bestChild;
            advanceToNextNode(curNode->actionToGetHere, curGs, ge);
        }
    }

    return curNode;
}

Action MctsBot::chooseAction(const GameEngine& ge, const GameState& gs) {
    MctsNode root;
    rng.seed(gs.currentRound  + gs.seed_ * 100);
    const auto bestChild = buildMcTree(root, gs, ge);

    return bestChild->actionToGetHere;
}

std::string renderMcTree(const MctsNode& node) {
    std::string ret;
    ret.reserve(100 * node.children.size());

    ret = "<table border=1><tr><td>depth: " + std::to_string(node.depth) + "<br>";
    for (const auto& child : node.children) {
        ret += "<details><summary>" + actionTypeToString(child.actionToGetHere.type) + " " + std::to_string(child.actionToGetHere.param) +
            " / " + std::to_string(child.stepIns) + " | " + std::to_string(child.aggPts) +
            " # " + std::to_string(child.pts) + "</summary>"
            + renderMcTree(child)
            + "</details>";
    }

    return ret + "</td></tr></table>";
}

bool allChildrenAreCalculated(const MctsNode& node) {
    for (const auto& child: node.children) {
        if (!child.nodeIsCompletelyEvaluated) {
            return false;
        }
    }

    return true;
}

const MctsNode* MctsBot::buildMcTree(MctsNode& root, const GameState& gs, const GameEngine& ge) const {
    Timer timer;
    
    [[maybe_unused]] volatile bool inCycle = false;

    double bottomPts = -123.0;

    // const MctsNode* prevBest = nullptr;
    // int lastBestChange = -1;

    for (int step = 0; step < steps; step++) {
        inCycle = true;
        MctsNode* bottom = goBottom(root, gs, ge);
        bottomPts = bottom->pts;

        while (bottom->parent != nullptr) {
            bottom->parent->aggPts = std::max(bottom->parent->aggPts, bottomPts);
            bottom->nodeIsCompletelyEvaluated = allChildrenAreCalculated(*bottom);

            bottom = bottom->parent;
        }

        // const MctsNode *bestChild = root.findBestChild();
        // if (bestChild != prevBest) {
        //     lastBestChange = step;
        //     prevBest = bestChild;
        // }

        if ((step & 15) == 15) {
            if (timer.elapsedMilliSeconds() > mctsTimeLimitMilliseconds) {
                // std::cout << "$";
                break;
            }
        }
    }

    // depthStats.at(gs.currentRound).push_back(lastBestChange);

    #ifdef DEBUG
    sumAndValidate(root);
    #endif

    const MctsNode *bestChild = root.findBestChild();
    
    // std::ofstream fout("logs/" + std::to_string(gs.seed_) + "_b" + std::to_string(steps) + "_r" + std::to_string(gs.currentRound) + ".html");
    // fout << "<html><body>" << renderMcTree(root) << "</body></html>";
    // fout.close();

    return bestChild;
}

// Resources MctsBot::chooseResourcesForMarket(const GameEngine& ge, const GameState& gs, int param) const {
//     const auto& ps = gs.players[gs.activePlayer];
//     Resources ret(ge.getRules().nResources);

//     int resourcesToMove = 0;
//     for (int grade = 0; grade < ps.nWorkers.size(); grade++) {
//         resourcesToMove += ge.getRules().workers.at(grade).marketPower * ps.nWorkers.at(grade);
//     }
//     resourcesToMove = std::min(ge.getRules().maxWorkersAtMarket, resourcesToMove);

//     if (param == 2) {
//         ret.amount[WinPoints] = resourcesToMove;
//         return ret;
//     }
//     if (param == 1) {
//         ret.amount[WinPoints] = resourcesToMove / 2;
//         resourcesToMove -= ret.amount[WinPoints];
//     }

//     Resources productionBalance(ge.getRules().nResources);
//     for (const auto& b: ps.buildings) {
//         productionBalance += b->out;
//         productionBalance -= b->in;
//     }
//     Resources upkeepBalance(ge.getRules().nResources);
//     for (const auto& b: ps.buildings) {
//         upkeepBalance += b->upkeep;
//     }
//     for (const auto& [w, idx]: enumerate(ps.nWorkers)) {
//         upkeepBalance += ge.getRules().workerUpkeepCost.at(idx) * w;
//     }
//     productionBalance = productionBalance * 2 - upkeepBalance;

//     // assert(resourcesToMove < 20);

//     if (abssum(productionBalance.amount) == 0) {
//         for (const auto& b: gs.basicBuildings) {
//             if (!b.empty()) productionBalance -= b.back()->price;
//         }
//         for (const auto& wc: ge.getRules().workerCards) {
//             productionBalance -= wc.price;
//         }
//     }

//     const auto marketPrices = argsort(gs.market.amount);
//     for (const auto& productId: marketPrices) {
//         // if (productId < 2) {
//         //     // Can't move money and winpoints are already moved
//         //     continue;
//         // }
//         if (productionBalance.amount.at(productId) > 0) {
//             int workers = std::min(resourcesToMove, gs.market.amount.at(productId));
//             ret.amount.at(productId) -= workers;
//             resourcesToMove -= workers;
//         } else if (productionBalance.amount.at(productId) < 0) {
//             int workers = std::min(resourcesToMove, ge.getRules().marketMaxResources - gs.market.amount.at(productId));
//             ret.amount.at(productId) += workers;
//             resourcesToMove -= workers;
//         }
//         if (resourcesToMove <= 0) {
//             break;
//         }
//     }
//     // assert(resourcesToMove == 0);

//     return ret;
// }

size_t MctsBot::chooseStartingWorkerCard(const GameEngine& ge, const GameState& gs, int param) const {
    MctsNode root;

    root.children.reserve(ge.getRules().workerCards.size());
    for (const auto& [wc, idx]: enumerate(ge.getRules().workerCards)) {
        
        const auto action = Action{ActionType::BuyWorkerForFree, (int) idx};
        GameState newState(gs);
        ge.doAction(action, newState);
        
        double pts = measureGs(GameCounters(ge, newState));
        // double pts = 0.0;
        root.children.emplace_back(MctsNode{
            action, &root, root.depth + 1, pts
        });
    }
    root.generatedChildren = true;

    const auto bestChild = buildMcTree(root, gs, ge);

    return bestChild->actionToGetHere.param;
}

void MctsBot::printInternalStats() const {
    for (const auto& [stats, r]: enumerate(depthStats)) {
        if (!stats.empty()) {
            std::cout << r << ": " << stats.size() << " playouts, mean: "  << mean(stats) << " | max: " << *std::max_element(stats.begin(), stats.end()) << std::endl;
        }
    }
}

double MctsBot::measureGs(const GameCounters& gc) const {
    assert(gc.curRound <= measurerParamsByRound.size());
    if (gc.gameEnded) {
        return gc.pureWp;
    }
    return ::measureGs(gc, measurerParamsByRound.at(gc.curRound));
}

std::pair<std::vector<MctsBot>, std::vector<IBot*>> toPtrVector(const std::vector<MctsBotParams>& botParams, const std::vector<std::vector<MeasurerParams>>& paramsByRound) {
	std::vector<MctsBot> bots;
	bots.reserve(botParams.size() * paramsByRound.size() * 2);
	std::vector<IBot*> botPtrs;
	for (const auto& mp: paramsByRound) {
        for (const auto& params: botParams) {
            bots.emplace_back(params, mp);
            botPtrs.emplace_back(&bots.back());

            bots.emplace_back(new GreedyBot(mp), params, mp);
            botPtrs.emplace_back(&bots.back());
        }
	}

	return {
		std::move(bots),
		std::move(botPtrs)
	};
}
