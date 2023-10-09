#include "ActionStats.h"

#include <sstream>

ActionStats::ActionStats(const std::vector<GameResult> &gr, double wpThresholdPercentile, bool onlyWinner)
    : buildingAmount(100, 0)
    , questAmount(100, 0)
{
    const auto winPoints = GameResult::avgWinPoints(gr);
    const auto wpThreshold = percentile(winPoints, wpThresholdPercentile);

    std::array<int, TotalActionTypes> actionStats{{0}};
    std::array<std::vector<double>, TotalActionTypes> actionStatsVectors;
    std::array<int, WpSourcesTotal> wpSourcesStats{{0}};
    std::array<std::vector<double>, WpSourcesTotal> wpSourcesStatsVectors;

    std::array<std::array<int, TotalActionTypes>, TotalActionTypes> actionAfterActionStat{{ {{0}} }};
//     std::vector<std::vector<Action>> actionsLog;
// std::vector<std::array<int, WpSourcesTotal>> wpSources;
// std::vector<std::array<std::array<int, 6>, 6>> actionAfterActionStat;

    for (const auto& res: gr) {
        for(const auto& [wp, pidx]: enumerate(res.winPoints)) {
            if(onlyWinner && res.winner != pidx) {
                continue;
            }

            if (wp >= wpThreshold) {
                totalGames++;

                const auto& state = res.stats.playerFinalState.at(pidx);
                for (const auto& b: state.buildings) {
                    buildingAmount.at(b->idx)++;
                }
                for (const auto& [q, qidx]: enumerate(state.questProgress)) {
                    if (q >= 1.0) questAmount.at(qidx)++;
                }

                const auto& curStats = res.stats.pis.at(pidx);
                std::array<size_t, TotalActionTypes> astats{{0}};
                for (const auto& s: curStats.actionsLog) {
                    actionStats.at(static_cast<int>(s.type))++;
                    astats.at(static_cast<int>(s.type))++;
                }

                for (int aidx = 0; aidx < TotalActionTypes; aidx++) {
                    actionStatsVectors.at(aidx).push_back(astats.at(aidx));
                }

                for (int sidx = 0; sidx < WpSourcesTotal; sidx++) {
                    wpSourcesStats.at(sidx) += res.stats.wpSources.at(pidx).at(sidx);
                    wpSourcesStatsVectors.at(sidx).push_back(res.stats.wpSources.at(pidx).at(sidx));
                }

                for (int s1idx = 0; s1idx < TotalActionTypes; s1idx++) {
                    for (int s2idx = 0; s2idx < TotalActionTypes; s2idx++) {
                        actionAfterActionStat.at(s1idx).at(s2idx) += 
                            curStats.actionAfterActionStat.at(s1idx).at(s2idx);
                    }
                }
            }
        }
    }

    for (auto& b: buildingAmount) {
        b /= totalGames;
    }
    for (auto& q: questAmount) {
        q /= totalGames;
    }

    // normalize
    for (int i = 0; i < actionStatsVectors.front().size(); i++) {
        double sum = 0.0;
        for (int aidx = 0; aidx < TotalActionTypes; aidx++) {
            sum += actionStatsVectors.at(aidx).at(i);
        }

        for (int aidx = 0; aidx < TotalActionTypes; aidx++) {
            actionStatsVectors.at(aidx).at(i) /= sum;
        }
    }
    
    for (int actionType = 0; actionType < TotalActionTypes; actionType++) {
        actionPct09.push_back(percentile(actionStatsVectors.at(actionType), 0.9));
        actionPct01.push_back(percentile(actionStatsVectors.at(actionType), 0.1));
    }

    // normalize
    for (int i = 0; i < wpSourcesStatsVectors.front().size(); i++) {
        double sum = 0.0;
        for (int source = 0; source < WpSourcesTotal; source++) {
            sum += wpSourcesStatsVectors.at(source).at(i);
        }

        for (int source = 0; source < WpSourcesTotal; source++) {
            wpSourcesStatsVectors.at(source).at(i) /= sum;
        }
    }
    for (int source = 0; source < WpSourcesTotal; source++) {
        wpSourcePct09.push_back(percentile(wpSourcesStatsVectors.at(source), 0.9));
        wpSourcePct01.push_back(percentile(wpSourcesStatsVectors.at(source), 0.1));
    }
}

std::string ActionStats::serialize() const {
    std::ostringstream ss;

    ss << std::setprecision(4);

    const auto buildingOrder = argsortDescending(buildingAmount);
    for (const auto& bIdx: buildingOrder) {
        if (buildingAmount.at(bIdx) > 0) {
            ss << bIdx << ": " << (double) buildingAmount.at(bIdx) << " | ";
        }
    }
    ss << "<br>" << std::endl;

    const auto questOrder = argsortDescending(questAmount);
    for (const auto& qIdx: questOrder) {
        if (questAmount.at(qIdx) > 0) {
            ss << qIdx << ": " << (double) questAmount.at(qIdx) << " | ";
        }
    }
    ss << "<br>" << std::endl;

    for (int actionType = 0; actionType < TotalActionTypes; actionType++) {
        ss << actionTypeToString(static_cast<ActionType>(actionType)) << ": " << actionPct09.at(actionType) << std::endl;
    }

    ss << "totalActionDiversity: " << sum(actionPct09) << std::endl;

    ss << "\n<br>Percentile wpSourcesStats:\n";

    for (int source = 0; source < WpSourcesTotal; source++) {
        ss << wpSourceToString(static_cast<WpSource>(source)) << ": " << wpSourcePct09.at(source) << std::endl;
    }
    ss << "wpSource diversity 0.9: " << sum(wpSourcePct09) << "<br>" << std::endl;

    for (int source = 0; source < WpSourcesTotal; source++) {
        ss << wpSourceToString(static_cast<WpSource>(source)) << ": " << wpSourcePct01.at(source) << std::endl;
    }
    ss << "wpSource diversity 0.1: " << sum(wpSourcePct01) << "<br>" << std::endl;

    return ss.str();
}
