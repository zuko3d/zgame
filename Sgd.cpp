#include "Sgd.h"

#include "RandomBot.h"
#include "MctsBot.h"
#include "Tournament.h"
#include "Timer.h"

#include <array>

MeasurerParams calcGradient(const GameCounters& gc, const MeasurerParams& params) {
    MeasurerParams ret;

    for (size_t qidx = 0; qidx < gc.questProgress.size(); qidx++) {
    // for (const auto& [p, qidx]: enumerate()) {
        ret.questCoef.at(qidx) = gc.questProgress.at(qidx);
    }

    for (const auto& b: gc.buildings) {
        ret.buildingCoef.at(b) = 1.0;
    }
    double buildingOnSalePoints = 0.0;
    for (const auto& b: gc.buildingsOnSale) {
        buildingOnSalePoints += params.buildingCoef.at(b);
        ret.buildingCoef.at(b) += params.buildingOnSaleCoef;
    }

    for (size_t ridx = 0; ridx < gc.nWorkers.size(); ridx++) {
        ret.workerCoef.at(ridx) = gc.nWorkers.at(ridx);
    }

    for (size_t ridx = 0; ridx < gc.productionResBalance.amount.size(); ridx++) {
        ret.resourceIncomeCoef.at(ridx) = gc.productionResBalance.amount.at(ridx);
    }

    // double pts = gc.pureWp * pow(gc.gameStage + 1e-6, params.wpPow) - 
    //     pow(1.0 - gc.gameStage + 1e-6, params.incomePow) * params.incomeCoef * gc.productionProfit +
    //     sqrt(std::max(0, gc.moneyWp)) * params.moneyCoef +
    //     questPoints * params.questCoef;
    // gc.buildingWp * params.buildingWpCoef +
    //     gc.workerWp * params.workerWpCoef
    // buildingPoints +
    //     buildingOnSalePoints * params.buildingOnSaleCoef;
    ret.incomeCoef = gc.productionProfit;
    ret.moneyCoef = std::max(0, gc.moneyWp);
    ret.wpPow = 0.0;
    ret.incomePow = 0.0;
    ret.buildingWpCoef = gc.buildingWp;
    ret.workerWpCoef = gc.workerWp;
    ret.buildingOnSaleCoef = buildingOnSalePoints;

    return ret;
}

double Sgd::evalNdcg(const std::vector<std::vector<double>>& y_true, const std::vector<std::vector<double>>& y_pred, double discount, double minShift) {
    double ret = 0.0;

    assert(y_true.size() == y_pred.size());

    for (int idx = 0; idx < y_pred.size(); ++idx) {
        const auto& curTrue = y_true.at(idx);
        double maxNdcg = 0.0;
        double min = curTrue.back();
        for (const auto& [pts, pos]: enumerate(curTrue)) {
            maxNdcg += pow(pts - min, minShift) * pow(discount, pos);
        }

        const auto order = argsortDescending(y_pred.at(idx));

        double ndcg = 0.0;
        for (const auto& [ord, pos]: enumerate(order)) {
            ndcg += pow(curTrue.at(ord) - min, minShift) * pow(discount, pos);
        }

        ret += ndcg / (maxNdcg + 1e-6);
        assert(std::isfinite(ret));
    }

    return ret / y_pred.size();
}

// mean reciprocal rank squared
double Sgd::evalMRRS(const std::vector<std::vector<double>>& y_true, const std::vector<std::vector<double>>& y_pred) {
    double ret = 0.0;

    assert(y_true.size() == y_pred.size());

    for (int idx = 0; idx < y_pred.size(); ++idx) {
        // const auto& curTrue = y_true.at(idx);
        const auto order = argsortDescending(y_pred.at(idx));

        ret += 1.0 / pow(order.front() + 1.0, 2.0);
    }

    return ret / y_pred.size();
}

double Sgd::evalPtsLoss(const std::vector<std::vector<double>>& y_true, const std::vector<std::vector<double>>& y_pred) {
    double ret = 0.0;

    assert(y_true.size() == y_pred.size());

    for (int idx = 0; idx < y_pred.size(); ++idx) {
        const auto& curTrue = y_true.at(idx);
        const auto& curPred = y_pred.at(idx);
        // const auto order = argsortDescending(y_pred.at(idx));
        const auto maxPos = std::max_element(curPred.begin(), curPred.end());

        ret += curTrue.at(std::distance(curPred.begin(), maxPos)) / curTrue.front();
    }

    return ret / y_pred.size();
}

MeasurerParams Sgd::trainPointwise(CountersDataset trainDataset, double lr, int seed, const std::array<double, WpSourcesTotal>& sourceWeights) {
    std::default_random_engine rng(seed);
    MeasurerParams ret(rng);
    double* retPtr = (double*) &ret;
    std::array<double, MeasurerParams::totalFields()> totalGrad;
    std::array<double, MeasurerParams::totalFields()> rmsGrad;
    std::array<double, MeasurerParams::totalFields()> gradMomentum;
    constexpr double betta1 = 0.9;
    constexpr double betta2 = 0.999;

    std::vector<std::vector<double>> y_true;
    rshuffle(trainDataset, rng);
    y_true.reserve(trainDataset.size());
    for (const auto& r: trainDataset) {
        y_true.emplace_back();
        for (const auto& [gc, pts]: r) {
            y_true.back().emplace_back(pts);
        }
    }

    for (auto& c: rmsGrad) {
        c = 1.0;
    }
    for (auto& c: gradMomentum) {
        c = 1.0;
    }

    double ewmaErr = 1.0;

    // std::cout << "Errors:\n";
    int n = 0;
    for (auto& c: totalGrad) {
        c = 0.0;
    }

    double prevMetric = 0.0;
    int stagnatedEpochs = 0;
    for (int epoch = 1; epoch < 5000; epoch++) {        
        Timer epochTimer;
        ewmaErr = 0.0;
        double mse = 0.0;
        std::vector<std::vector<double>> y_pred;

        for (const auto& gameCounters: trainDataset) {
            y_pred.emplace_back();
            y_pred.reserve(gameCounters.size());

            for (const auto& [gc, pts]: gameCounters) {
                const auto grad = calcGradient(gc, ret);
                double* gradPtr = (double*) &grad;
                const double expectedPts = measureGs(sourceWeights, gc, ret);
                y_pred.back().push_back(expectedPts);
                const double err = expectedPts - pts;
                mse += err * err;
                for (int idx = 0; idx < totalGrad.size(); idx++) {
                    gradPtr[idx] *= err;
                    // if (err < 0){
                    //     gradPtr[idx] *= -1;
                    // }

                    gradMomentum[idx] = betta1 * gradMomentum[idx] + (1.0 - betta1) * gradPtr[idx];
                    rmsGrad[idx] = betta2 * rmsGrad[idx] + (1.0 - betta2) * gradPtr[idx] * gradPtr[idx];
                    assert(std::isfinite(rmsGrad[idx]));
                    gradPtr[idx] = gradMomentum[idx] / (1.0 - pow(betta1, epoch)) / 
                        (sqrt(rmsGrad[idx] / pow(betta2, epoch)) + 1e-5);
                }

                // const size_t pos = rng() % MeasurerParams::totalFields();
                n++;
                for (int idx = 0; idx < totalGrad.size(); idx++) {
                    totalGrad[idx] -= gradPtr[idx];
                }
                // if (err < 0) {
                //     for (int idx = 0; idx < totalGrad.size(); idx++) {
                //         totalGrad[idx] += gradPtr[idx];
                //     }
                // } else if (err > 0) {
                //     for (int idx = 0; idx < totalGrad.size(); idx++) {
                //         totalGrad[idx] -= gradPtr[idx];
                //     }
                // }
                

                // for (int idx = 0; idx < totalGrad.size(); idx++) {
                //     MeasurerParams localRet(ret);
                //     double* localRetPtr = (double*) &localRet;
                //     double sgn = err / fabs(err);
                //     double dx = -gradPtr[idx] * 1e-5 * sgn;
                //     localRetPtr[idx] += dx;
                //     const double expectedPts2 = measureGs(gc, localRet);
                //     (void) expectedPts2;
                //     const auto df = expectedPts2 - expectedPts;
                //     (void) df;
                //     const auto df_dx = df / dx - gradPtr[idx];
                //     (void) df_dx;
                //     assert((dx == 0.0) || (df * err < 0));
                // }

                // ewmaErr = 0.999 * ewmaErr + 0.001 * fabs(err / pts);
                ewmaErr += fabs(err / pts);

                if (n == 16) {
                    Timer updateTimer;
                    // double gradNorm = 0.0;
                    for (auto& c: totalGrad) {
                        c /= n;
                        // gradNorm += c * c;
                    }
                    // gradNorm = sqrt(gradNorm);
                    // gradNorm /= totalGrad.size();
                    // // std::cout << gradNorm << std::endl;
                    // for (auto& c: totalGrad) {
                    //     c /= gradNorm;
                    // }
                    
                    for (int idx = 0; idx < totalGrad.size(); idx++) {
                        retPtr[idx] += totalGrad[idx] * lr;
                        // retPtr[idx] *= 1.0 - sqrt(lr);
                        if (retPtr[idx] < 0) {
                            retPtr[idx] += lr / (15.0 + fabs(totalGrad[idx]));
                        } else {
                            retPtr[idx] -= lr / (15.0 + fabs(totalGrad[idx]));
                        }
                    }

                    assert(abssum(ret.buildingCoef) < 1e8);

                    for (auto& c: totalGrad) {
                        c = 0.0;
                    }

                    n = 0;
                    // updateTime += updateTimer.elapsedUSeconds();
                    // std::cout << "updateTimer: " << updateTimer.elapsedUSeconds() << " usec\n";
                }
            }
        }

        ewmaErr /= trainDataset.size();

        for (int idx = 0; idx < totalGrad.size(); idx++) {
            if (fabs(retPtr[idx]) < lr * 0.1) {
                retPtr[idx] = 0.0;
            }
        }
        // lr *= 0.9999;

        double metric = evalMRRS(y_true, y_pred);
        if (metric <= prevMetric + 1e-5) {
            stagnatedEpochs++;
        } else {
            stagnatedEpochs = 0;
            prevMetric = metric;
        }

        // if (epoch % 1 == 0) {
        if (stagnatedEpochs >= 4) {
            // std::cout << "---\n";
            std::cout << "finished at epoch " << epoch << ": " << ewmaErr << "\tmse: " << sqrt(mse / trainDataset.size())
                << "\tndcg_min: " << evalNdcg(y_true, y_pred, 0.5, 2.5) 
                << "\tevalPtsLoss: " << evalPtsLoss(y_true, y_pred)
                << "\tevalMRRS: " << evalMRRS(y_true, y_pred)
                << std::endl;
            // std::cout << "lr: " << lr << std::endl;
            // std::cout << ret.serialize() << std::endl;
            break;
        }

        // std::cout << "updateTime: " << updateTime << " usec\n";
        // std::cout << "epochTimer: " << epochTimer.elapsedMilliSeconds() << " msec\n";
    }
    // std::cout << "ewmaErr: " << ewmaErr << std::endl;
    return ret;
}

MeasurerParams Sgd::trainPairwise(CountersDataset trainDataset, double lr, int seed, const std::array<double, WpSourcesTotal>& sourceWeights) {
    std::default_random_engine rng(seed);
    MeasurerParams ret(rng);
    double* retPtr = (double*) &ret;
    std::array<double, MeasurerParams::totalFields()> totalGrad;
    std::array<double, MeasurerParams::totalFields()> rmsGrad;
    std::array<double, MeasurerParams::totalFields()> gradMomentum;
    constexpr double betta1 = 0.9;
    constexpr double betta2 = 0.999;

    std::vector<std::vector<double>> y_true;
    rshuffle(trainDataset, rng);
    y_true.reserve(trainDataset.size());
    for (const auto& r: trainDataset) {
        y_true.emplace_back();
        for (const auto& [gc, pts]: r) {
            y_true.back().emplace_back(pts);
        }
    }

    for (auto& c: rmsGrad) {
        c = 1.0;
    }
    for (auto& c: gradMomentum) {
        c = 1.0;
    }

    // std::cout << "Errors:\n";
    int n = 0;
    for (auto& c: totalGrad) {
        c = 0.0;
    }

    double prevMetric = 0.0;
    int stagnatedEpochs = 0;
    for (int epoch = 1; epoch < 5000; epoch++) {        
        std::vector<std::vector<double>> y_pred;

        for (const auto& gameCounters: trainDataset) {
            y_pred.emplace_back();
            y_pred.reserve(gameCounters.size());

            // winner
        
            const auto& [winnerGc, winnerPts] = gameCounters.front();
            const auto winnerGrad = calcGradient(winnerGc, ret);
            double* winnerGradPtr = (double*) &winnerGrad;
            const double winnerExpectedPts = measureGs(sourceWeights, winnerGc, ret);
            y_pred.back().push_back(winnerExpectedPts);

            for (int pos = 1; pos < gameCounters.size(); pos++) {
                const auto& [gc, pts] = gameCounters.at(pos);

                const auto grad = calcGradient(gc, ret);
                double* gradPtr = (double*) &grad;
                const double expectedPts = measureGs(sourceWeights, gc, ret);
                y_pred.back().push_back(expectedPts);

                const double err = expectedPts - winnerExpectedPts;
                if (err >= 0) {
                    // for (int idx = 0; idx < totalGrad.size(); idx++) {
                    //     totalGrad[idx] += winnerGradPtr[idx];
                    // }

                    for (int idx = 0; idx < totalGrad.size(); idx++) {
                        gradPtr[idx] -= winnerGradPtr[idx];
                        gradPtr[idx] *= err;
                        gradMomentum[idx] = betta1 * gradMomentum[idx] + (1.0 - betta1) * gradPtr[idx];
                        rmsGrad[idx] = betta2 * rmsGrad[idx] + (1.0 - betta2) * gradPtr[idx] * gradPtr[idx];
                        assert(std::isfinite(rmsGrad[idx]));
                        gradPtr[idx] = gradMomentum[idx] / (1.0 - pow(betta1, epoch)) / 
                            (sqrt(rmsGrad[idx] / pow(betta2, epoch)) + 1e-5);
                    }

                    n++;
                    for (int idx = 0; idx < totalGrad.size(); idx++) {
                        totalGrad[idx] -= gradPtr[idx];
                    }
                    assert(abssum(grad.buildingCoef) < 1e7);
                }

                if (n >= 32) {
                    for (auto& c: totalGrad) {
                        c /= n;
                    }
                    
                    for (int idx = 0; idx < totalGrad.size(); idx++) {
                        retPtr[idx] += totalGrad[idx] * lr;
                        // retPtr[idx] *= 1.0 - sqrt(lr);
                        if (retPtr[idx] < 0) {
                            retPtr[idx] += lr / (30.0 + fabs(totalGrad[idx]));
                        } else {
                            retPtr[idx] -= lr / (30.0 + fabs(totalGrad[idx]));
                        }
                    }

                    assert(abssum(ret.buildingCoef) < 1e8);

                    for (auto& c: totalGrad) {
                        c = 0.0;
                    }

                    n = 0;
                }
            }
        }

        for (int idx = 0; idx < totalGrad.size(); idx++) {
            if (fabs(retPtr[idx]) < lr * 0.1) {
                retPtr[idx] = 0.0;
            }
        }
        // lr *= 0.9999;

        double metric = evalMRRS(y_true, y_pred);
        if (metric <= prevMetric + 1e-5) {
            stagnatedEpochs++;
        } else {
            stagnatedEpochs = 0;
            prevMetric = metric;
        }

        // if (epoch % 1 == 0) {
        if (stagnatedEpochs >= 4) {
            // std::cout << "---\n";
            std::cout << "finished at epoch " << epoch << ": "
                << "\tndcg_min: " << evalNdcg(y_true, y_pred, 0.5, 2.5) 
                << "\tevalPtsLoss: " << evalPtsLoss(y_true, y_pred)
                << "\tevalMRRS: " << evalMRRS(y_true, y_pred)
                << std::endl;
            // std::cout << "lr: " << lr << std::endl;
            // std::cout << ret.serialize() << std::endl;
            break;
        }

        // std::cout << "updateTime: " << updateTime << " usec\n";
        // std::cout << "epochTimer: " << epochTimer.elapsedMilliSeconds() << " msec\n";
    }
    // std::cout << "ewmaErr: " << ewmaErr << std::endl;
    return ret;
}

MeasurerParams Sgd::learnMeasurer(const GameRules& rules, int seed, int round, int timelimitSeconds, const MeasurerParams& measurerParams, int focusedWpSource, double focusCoef) {
    std::default_random_engine rng(seed);

    CountersDataset gameCounters;
    gameCounters.reserve(10000000);

    std::array<double, WpSourcesTotal> sourceWeights;
    for (auto& w: sourceWeights) {
        w = 1.0;
    }
    if (focusedWpSource >= 0) {
        sourceWeights.at(focusedWpSource) = focusCoef;
    }
 
    std::vector<IBot*> basic {
        new RandomBot(seed * 100, measurerParams),
        new RandomBot(seed * 100 + 1, measurerParams),
        new RandomBot(seed * 100 + 2, measurerParams),
        new RandomBot(seed * 100 + 3, measurerParams),
        new RandomBot(seed * 100 + 4, measurerParams),
        new RandomBot(seed * 100 + 5, measurerParams),
        new RandomBot(seed * 100 + 6, measurerParams),
        new RandomBot(seed * 100 + 7, measurerParams),
    };

    // std::vector<MeasurerParams> measurerParamsByRound(rules.totalRounds);
    // std::vector<IBot*> basic {
    //     new MctsBot(nullptr, 1000, 10, MctsBotParams{rules.totalRounds, 0.01}, measurerParamsByRound),
    //     new MctsBot(nullptr, 1000, 10, MctsBotParams{rules.totalRounds, 0.3}, measurerParamsByRound),
    //     new MctsBot(nullptr, 1000, 10, MctsBotParams{rules.totalRounds, 0.6}, measurerParamsByRound),
    //     new MctsBot(nullptr, 1000, 10, MctsBotParams{rules.totalRounds, 0.9}, measurerParamsByRound),
    //     new MctsBot(nullptr, 1000, 10, MctsBotParams{rules.totalRounds, 1.2}, measurerParamsByRound),
    //     new MctsBot(nullptr, 1000, 10, MctsBotParams{rules.totalRounds, 1.5}, measurerParamsByRound),
    //     new MctsBot(nullptr, 1000, 10, MctsBotParams{rules.totalRounds, 1.7}, measurerParamsByRound),
    //     new MctsBot(nullptr, 1000, 10, MctsBotParams{rules.totalRounds, 2.0}, measurerParamsByRound),
    // };

    const auto basicResults = Tournament::playSP(EngineTrainParams{sourceWeights, &gameCounters, round}, rules, basic, 1000000, 0, true, timelimitSeconds);
    const auto basicWinPoints = GameResult::avgWinPoints(basicResults);
    CountersDataset trainSet;
    trainSet.reserve(gameCounters.size());

    for (auto& gc: gameCounters) {
        std::sort(gc.begin(), gc.end(),
            [](const std::pair<GameCounters, double>& op1, const std::pair<GameCounters, double>& op2) {
                return op1.second > op2.second;
            });
    }

    std::cout << ActionStats(basicResults, 0.7).serialize();
    
    std::cout << "collected gameCounters: " << gameCounters.size() << std::endl;

    std::vector<double> points;
    points.reserve(gameCounters.size());
    for (const auto& gc: gameCounters) {
        points.push_back(gc.front().second);
    }
    const auto order = argsort(points);
    auto riter = order.rbegin();
    const auto totalSamples = std::min(30000, (int) gameCounters.size());
    for (int idx = 0; idx < totalSamples; idx++) {
        trainSet.emplace_back(gameCounters.at(*riter));
        ++riter;
    }
    std::cout << "min train pts:" << trainSet.back().front().second << std::endl;

    const MeasurerParams ret = Sgd::trainPointwise(trainSet, 1e-2, seed, sourceWeights);
    return ret;
}
