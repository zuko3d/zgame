#include "GreedyBot.h"

#include "GameEngine.h"
#include "GameCounters.h"

#include <cmath>

GreedyBot::GreedyBot(std::vector<MeasurerParams> measurerParamsByRound_)
    : IBot({})
    , measurerParamsByRound(std::move(measurerParamsByRound_))
{ }

Action GreedyBot::chooseAction(const GameEngine& ge, const GameState& gs) {
    auto actions = ge.generateActions(gs);

    double bestPts = -1e9;
    Action bestAction{ActionType::Borrow, -2};

    for (const auto& action: actions) {
        GameState newState(gs);
        ge.doAction(action, newState);
        ge.doAfterTurnActions(newState);

        double pts = measureGs(GameCounters(ge, newState));

        if (pts > bestPts) {
            bestPts = pts;
            bestAction = action;
        }
    }

    return bestAction;
}
