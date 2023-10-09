#pragma once

#include "GameCounters.h"
#include "Types.h"

#include <vector>

class Sgd {
public:
    static double evalPtsLoss(const std::vector<std::vector<double>>& y_true, const std::vector<std::vector<double>>& y_pred);
    static double evalMRRS(const std::vector<std::vector<double>>& y_true, const std::vector<std::vector<double>>& y_pred);
    static double evalNdcg(const std::vector<std::vector<double>>& y_true, const std::vector<std::vector<double>>& y_pred, double discount, double minShift);

    static MeasurerParams trainPointwise(CountersDataset trainDataset, double lr, int seed, const std::array<double, WpSourcesTotal>& sourceWeights);
    static MeasurerParams trainPairwise(CountersDataset trainDataset, double lr, int seed, const std::array<double, WpSourcesTotal>& sourceWeights);
    static MeasurerParams learnMeasurer(const GameRules& rules, int seed, int round, int timelimitSeconds, const MeasurerParams& measurerParams, int focusedWpSource, double focusCoef);
};
