#include "Quest.h"

#include "PlayerState.h"

#include <assert.h>

double Quest::checkProgress(const PlayerState& ps) const {
    double totalScore = 0.0;
    for (const auto& req: reqs) {
        double score = 0.0;
        switch (req.type) {
            case QuestRequirementType::GoldAmount:
                score += ((double) std::max(0, ps.resources.amount[Money])) / req.paramAmount;
                break;
            case QuestRequirementType::ExactBuildingColorAmount :
                score += ((double) ps.buildingsByColor.at(req.paramType)) / req.paramAmount;
                break;
            case QuestRequirementType::TotalBuildingsAmount :
                score += ((double) ps.buildings.size())  / req.paramAmount;
                break;
            case QuestRequirementType::UniqueBuildingColors :
                score += ((double) ps.uniqueBuildingColors) / req.paramAmount;
                break;
            case QuestRequirementType::TotalWorkersAmount:
                score += ((double) sum(ps.nWorkers)) / req.paramAmount;
                break;
            case QuestRequirementType::UniqueWorkerTypes:
                score += ((double) ps.uniqueWorkerTypes) / req.paramAmount;
                break;
            case QuestRequirementType::ExactWorkerTypeAmount:
                score += ((double) ps.nWorkers.at(req.paramType)) / req.paramAmount;
                break;
        }
        totalScore += std::min(score, 1.0);
    }

    assert (totalScore <= reqs.size());
    return totalScore / reqs.size();
}