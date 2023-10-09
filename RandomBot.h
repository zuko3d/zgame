#pragma once

#include "IBot.h"

#include <random>

class RandomBot : public IBot {
public:
    RandomBot(int seed_, const MeasurerParams& mp = MeasurerParams{});

    virtual Action chooseAction(const GameEngine& ge, const GameState& gs);

    virtual std::string hashState() { return std::to_string(seed) + measurerParams.serialize(); }

private:
    std::default_random_engine rng;
    int seed;
};
