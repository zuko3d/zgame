#include "Trainers.h"

#include "Timer.h"
#include "Tournament.h"
#include "Sgd.h"
#include "MctsBot.h"

std::vector<MeasurerParams> trainParamsByRound(const GameRules& rules, const std::vector<MctsBotParams>& mctsBotParams) {
    std::vector<MeasurerParams> paramsByRound(rules.totalRounds);
    Timer fullParamsByRound;
    for (int round = rules.totalRounds - 2; round >= 0; round--) {
    // for (int round = 21; round >= 0; round--) {
        std::cout << "=======\n";
        std::cout << "round: " << round << std::endl;

        MeasurerParams avgMp;
        double* avgMpPtr = (double*) &avgMp;
        for (int i = 0; i < MeasurerParams::totalFields(); i++) {
            avgMpPtr[i] = 0;
        }
        const int retries = 3;
        for (int seed = 0; seed < retries; seed++) {
            Timer learnTimer;
            const auto mp = Sgd::learnMeasurer(rules,round * 1000 + seed, round, 50, paramsByRound.at(std::min(round + MeasurerLearnerDepth, rules.totalRounds - 1)), -1, 1);
            double* mpPtr = (double*) &mp;
            for (int i = 0; i < MeasurerParams::totalFields(); i++) {
                avgMpPtr[i] += mpPtr[i];
            }
            std::cout << "elapsed: " << learnTimer.elapsedSeconds() << std::endl;
        }

        for (int i = 0; i < MeasurerParams::totalFields(); i++) {
            avgMpPtr[i] /= retries;
        }
        std::cout << avgMp.serialize() << std::endl;
        paramsByRound.at(round) = avgMp;
    }
    std::cout << "fullParamsByRound time: " << fullParamsByRound.elapsedSeconds() << " seconds" << std::endl;

    const auto [cbots, cbotPtrs] = toPtrVector(mctsBotParams, {paramsByRound});
    const auto basicResults = Tournament::playSP(EngineTrainParams{}, rules, cbotPtrs, 60, 0, true, 600);
    const auto basicWinPoints = GameResult::avgWinPoints(basicResults);

    for (const auto& [wp, idx]: enumerate(basicWinPoints)) {
        std::cout << idx << ": " << wp << std::endl;
    }

    std::cout << ActionStats(basicResults).serialize();

    return paramsByRound;
}

void tuneWpSource(GameRules& rules, WpSource source, double magnitude, int seed) {
    std::default_random_engine rng(seed);

    // magnitude == 1 means no changes
    // magnitude > 1 means buffs, <1 means nerfs
    switch (source) {
        case WpSource::BuildingWp: {
            for (auto& b: rules.basicBuildings) {
                if (b.winPoints > 0) {
                    if (magnitude > 1) {
                        b.winPoints += lnrand(rng) * magnitude;
                    } else {
                        b.winPoints = std::max((int) 1, (int) (b.winPoints - lnrand(rng) / magnitude));
                    }
                }
                b.tunePrice(1.0 / magnitude);
            }
            break;
        }
        case WpSource::FinalGoldWp: {
            rules.moneyPerWp /= magnitude;
            break;
        }
        case WpSource::MarketDeployWp: {
            rules.marketWpCoef *= magnitude;
            break;
        }
        case WpSource::ProductionWp: {
            for (auto& b: rules.basicBuildings) {
                if (b.out.amount[WinPoints] > 0) {
                    if (magnitude > 1) {
                        b.out.amount[WinPoints] += lnrand(rng) * magnitude;
                    } else {
                        b.out.amount[WinPoints] = std::max(1, (int) (b.out.amount[WinPoints] - lnrand(rng) / magnitude));
                    }
                }
                b.tunePrice(1.0 / magnitude);
            }
            break;
        }
        case WpSource::QuestWp: {
            for (auto& q: rules.quests) {
                if (magnitude > 1) {
                    q.winPoints += lnrand(rng) * magnitude;
                } else {
                    q.winPoints = std::max((int) 1, (int) (q.winPoints - lnrand(rng) / magnitude));
                }
            }
            break;
        }
        case WpSource::WorkerWp: {
            for (auto& w: rules.workers) {
                if (w.winPoints > 0) {
                    if (magnitude > 1) {
                        w.winPoints += lnrand(rng) * magnitude;
                    } else {
                        w.winPoints = std::max(1, (int) (w.winPoints - lnrand(rng) / magnitude));
                    }
                }
            }            
            break;
        }
        case WpSource::WpSourcesTotal:
            break;
    }
}

void tuneRules(GameRules& rules, const ActionStats& actionStats, int seed) {
    for (int source = 0; source < WpSourcesTotal; source++) {
        if (actionStats.wpSourcePct01[source] > 1.5 / (double) WpSourcesTotal) {
            tuneWpSource(rules, (WpSource) source, 1.01 - actionStats.wpSourcePct01[source], seed);
        }
        if (actionStats.wpSourcePct09[source] < 0.9 / (double) WpSourcesTotal) {
            tuneWpSource(rules, (WpSource) source, 1.4, seed);
        }
    }
    rules.tuneVersion++;
}
