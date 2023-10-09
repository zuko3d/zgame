#pragma once

#include "Tournament.h"
#include "GameRules.h"
#include "Timer.h"

#include <random>
#include <iostream>
#include <vector>

template <class BotClass, class ParamClass>
class Breeding {
public:
    Breeding(const GameRules& rules_, int nVillages, int villagePop_, int seed)
        : rules(rules_)
        , rng(seed)
        , villagePop(villagePop_)
    {
        villages.resize(nVillages);
    }

    // ParamClass sgd(ParamClass params) {
    //     const auto variants = params.genMutations(urand(rng, 0.1, 0.3));
    //     std::vector<ParamClass> village;
    //     village.push_back(params);
    //     double baselineWp = 0.0;
    //     {
    //         const auto [bots, botPtrs] = toPtrVector({params});
    //         const auto results = Tournament::playSP(rules, botPtrs, 10, 0, true);
    //         const auto winPoints = GameResult::avgWinPoints(results);
    //         double wp = winPoints.front();
    //         if (baselineWp < wp) {
    //             baselineWp = wp;
    //         }
    //     }
    //     for (const auto& [newParams, mut]: variants) {
    //         const auto [bots, botPtrs] = toPtrVector({params});
    //         const auto results = Tournament::playSP(rules, botPtrs, 10, 0, true);
    //         const auto winPoints = GameResult::avgWinPoints(results);
    //         double wp = winPoints.front();
    //         if (baselineWp < wp) {
    //             baselineWp = wp;
    //         }
    //     }

    //     std::cout << "baselineWp: " << baselineWp << std::endl;
    //     for (int i = 1; i < winPoints.size(); i++) {
    //         std::cout << winPoints.at(i) << std::endl;
    //         const auto& mut = variants.at(i - 1).second;
    //         if (winPoints.at(i) > baselineWp) {
    //             params.applyMutation(mut.paramIdx, mut.change);
    //         } else {
    //             params.applyMutation(mut.paramIdx, 1.0 / mut.change);
    //         }
    //     }

    //     const auto [bot, botPtr] = toPtrVector({params});
    //     std::cout << "After sgd: " << GameResult::avgWinPoints(Tournament::playSP(rules, botPtr, 10, 0, true)).front()<< std::endl;

    //     return params;
    // }

    void breed(const std::vector<ParamClass>& heroes) {
        std::cout << "initial ======================" << std::endl;
        
        const auto [heroBots, heroPtrs] = toPtrVector(heroes);
        for (int vidx = 0; vidx < villages.size(); vidx++) {
            auto& village = villages[vidx];
            std::vector<ParamClass> nextOffspring;
            for (int i = 0; i < villagePop * 5; i++) {
                nextOffspring.emplace_back(ParamClass::generate(rng));
            }
            const auto [bots, botPtrs] = toPtrVector(nextOffspring);
            const auto [winrates, winPoints] = Tournament::playTeamVTeam(rules, botPtrs, heroPtrs, 20 / heroes.size());
            auto best = argsort(winrates);
            std::reverse(best.begin(), best.end());

            best.resize(villagePop);
            village.clear();
            std::cout << "vidx " << vidx << ": " << winrates[best[0]] << " | " << winPoints[best[0]] << " | " << 
                "vmean: " << mean(winPoints) << " vmed: " << median(winPoints) << " | " <<
                nextOffspring[best[0]].serialize() << std::endl;

            for (const auto idx: best) {
                village.push_back(nextOffspring[idx]);
            }            
        }

        for (int gen = 0; gen < 600; gen++) {
            std::cout << "generation " << gen << " ======================" << std::endl;
// #pragma omp parallel for schedule(dynamic)
            for (int vidx = 0; vidx < villages.size(); vidx++) {
                auto& village = villages[vidx];
                // std::cout << "village size: " << village.size() << std::endl;
                std::vector<ParamClass> nextOffspring;
                for (const auto& v: village) {
                    nextOffspring.push_back(v);
                    for (int i = 0; i < 3; i++) {
                        nextOffspring.push_back(v);
                        nextOffspring.back().crossover(rng, village[rng() % village.size()]);
                        nextOffspring.back().mutate(rng, i);
                    }
                }

                const auto [bots, botPtrs] = toPtrVector(nextOffspring);
                // std::vector<BotClass> bots;
                // bots.reserve(nextOffspring.size());
                // std::vector<IBot*> botPtrs;
                // for (const auto& params: nextOffspring) {
                //     bots.emplace_back(params);
                //     botPtrs.emplace_back(&bots.back());
                // }
                const auto [winrates, winPoints] = Tournament::playAllInAll(rules, botPtrs, 100 / botPtrs.size());
                auto best = argsort(winrates);
                std::reverse(best.begin(), best.end());

                best.resize(villagePop);
                village.clear();
                std::cout << "vidx " << vidx << ": " << winrates[best[0]] << " | " << winPoints[best[0]] << " | " << 
                    "vmean: " << mean(winPoints) << " vmed: " << median(winPoints) << " | " <<
                    nextOffspring[best[0]].serialize() << std::endl;

                for (const auto idx: best) {
                    village.push_back(nextOffspring[idx]);
                }
            }
        }
    }

    void breedSinglePlayer(const std::vector<MeasurerParams>& paramsByRound) {
        std::cout << "initial ======================" << std::endl;
        // villages.resize(nVillages);
        
        for (int vidx = 0; vidx < villages.size(); vidx++) {
            auto& village = villages[vidx];
            std::vector<ParamClass> nextOffspring;
            for (int i = 0; i < villagePop * 2; i++) {
                nextOffspring.emplace_back(ParamClass::generate(rng));
            }
            const auto [bots, botPtrs] = toPtrVector(nextOffspring, {paramsByRound});
            // Timer sp1timer;
            const auto results = Tournament::playSP(EngineTrainParams{}, rules, botPtrs, 50, 0, true, /* timeLimitSeconds = */ 600);
            // std::cout << "results: " << results.size() << ", " << results.size() / botPtrs.size() << " per bot, elapsed: " << sp1timer.elapsedSeconds() << std::endl;
            
            const auto winPoints = GameResult::avgWinPoints(results);
            auto best = argsort(winPoints);
            std::reverse(best.begin(), best.end());
            // GameResult::actionStats(results);

            best.resize(villagePop);
            village.clear();
            std::cout << "vidx " << vidx << ": == " << winPoints[best[0]] << " == " <<
                "\t10%: " << GameResult::percentileWinPoints(results, 0.1).at(best.front()) <<
                "\ttrueskill: " << GameResult::trueSkill(results).at(best.front()) <<
                "\tvmean: " << mean(winPoints) << " vmed: " << median(winPoints) << " | " <<
                nextOffspring[best[0]].serialize() << std::endl;

            for (const auto idx: best) {
                village.push_back(nextOffspring[idx]);
            }

            Timer sp2timer;
            const auto [vbots, vbotPtrs] = toPtrVector(village, {paramsByRound});
            const auto basicResults = Tournament::playSP(EngineTrainParams{}, rules, vbotPtrs, 60, 452529, true, 600);
            std::cout << "basicResults: " << basicResults.size() << ", " << basicResults.size() / vbotPtrs.size() << " per bot, elapsed: " << sp2timer.elapsedSeconds() << std::endl;
            const auto basicWinPoints = GameResult::avgWinPoints(basicResults);
            std::cout << "basicWps " << vidx <<" ======================" << std::endl;
            for (const auto& [wp, idx]: enumerate(basicWinPoints)) {
                std::cout << idx << ": " << wp << std::endl;
            }
            std::cout << "percentileWinPoints " << vidx <<" ++++++++" << std::endl;
            const auto percentileWinPoints = GameResult::percentileWinPoints(basicResults, 0.1);
            for (const auto& [wp, idx]: enumerate(percentileWinPoints)) {
                std::cout << idx << ": " << wp << std::endl;
            }
            std::cout << "trueSkill " << vidx << " ---------" << std::endl;
            const auto trueSkill = GameResult::trueSkill(basicResults);
            for (const auto& [wp, idx]: enumerate(trueSkill)) {
                std::cout << idx << ": " << wp << std::endl;
            }
        }

        for (int gen = 0; gen < 10; gen++) {
            std::cout << "generation " << gen << " ======================" << std::endl;

            Timer timer;
            for (int vidx = 0; vidx < villages.size(); vidx++) {
                auto& village = villages[vidx];

                std::vector<ParamClass> nextOffspring;
                for (const auto& v: village) {
                    // nextOffspring.push_back(sgd(v));

                    nextOffspring.push_back(v);
                    for (int i = 0; i < 2; i++) {
                        nextOffspring.push_back(v);
                        nextOffspring.back().crossover(rng, village[rng() % village.size()]);
                        nextOffspring.back().mutate(rng, i);
                    }
                }

                const auto [bots, botPtrs] = toPtrVector(nextOffspring, {paramsByRound});
                // std::vector<BotClass> bots;
                // bots.reserve(nextOffspring.size());
                // std::vector<IBot*> botPtrs;
                // for (const auto& params: nextOffspring) {
                //     bots.emplace_back(params);
                //     botPtrs.emplace_back(&bots.back());
                // }
                const auto results = Tournament::playSP(EngineTrainParams{}, rules, botPtrs, 50, 0, true, 600);
                const auto winPoints = GameResult::avgWinPoints(results);
                auto best = argsort(winPoints);
                std::reverse(best.begin(), best.end());
                // GameResult::actionStats(results);

                best.resize(villagePop);
                village.clear();
                std::cout << "vidx " << vidx << ": == " << winPoints[best[0]] << " == " <<
                    "\t10%: " << GameResult::percentileWinPoints(results, 0.1).at(best.front()) <<
                    "\ttrueskill: " << GameResult::trueSkill(results).at(best.front()) <<
                    "\tvmean: " << mean(winPoints) << " vmed: " << median(winPoints) << " | " <<
                    nextOffspring[best[0]].serialize() << std::endl;

                for (const auto idx: best) {
                    village.push_back(nextOffspring[idx]);
                }

                Timer sp2timer;
                const auto [vbots, vbotPtrs] = toPtrVector(village, {paramsByRound});
                const auto basicResults = Tournament::playSP(EngineTrainParams{}, rules, vbotPtrs, 60, 452529, true, 600);
                std::cout << "basicResults: " << basicResults.size() << ", " << basicResults.size() / vbotPtrs.size() << " per bot, elapsed: " << sp2timer.elapsedSeconds() << std::endl;
                const auto basicWinPoints = GameResult::avgWinPoints(basicResults);
                std::cout << "basicWps " << vidx <<" ======================" << std::endl;
                for (const auto& [wp, idx]: enumerate(basicWinPoints)) {
                    std::cout << idx << ": " << wp << std::endl;
                }
                std::cout << "percentileWinPoints " << vidx <<" ++++++++" << std::endl;
                const auto percentileWinPoints = GameResult::percentileWinPoints(basicResults, 0.1);
                for (const auto& [wp, idx]: enumerate(percentileWinPoints)) {
                    std::cout << idx << ": " << wp << std::endl;
                }
                std::cout << "trueSkill " << vidx << " ---------" << std::endl;
                const auto trueSkill = GameResult::trueSkill(basicResults);
                for (const auto& [wp, idx]: enumerate(trueSkill)) {
                    std::cout << idx << ": " << wp << std::endl;
                }
            }

            std::cout << "generation " << gen << " took " << timer.elapsedSeconds() << " seconds " << std::endl;
        }
    }

private:
    const GameRules& rules;
    std::default_random_engine rng;
    std::vector<std::vector<ParamClass>> villages;
    int villagePop;
};
