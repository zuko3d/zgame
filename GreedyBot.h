#pragma once

#include "IBot.h"

#include <random>

class GreedyBot : public IBot {
public:
    GreedyBot(std::vector<MeasurerParams> measurerParamsByRound_);

    virtual Action chooseAction(const GameEngine& ge, const GameState& gs);

protected:
    std::vector<MeasurerParams> measurerParamsByRound;
};
