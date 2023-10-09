#include "RandomBot.h"

#include "GameEngine.h"

#include <stdlib.h>

RandomBot::RandomBot(int seed_, const MeasurerParams& mp)
    : IBot(mp)
    , rng(seed_)
    , seed(seed_)
{ }

Action RandomBot::chooseAction(const GameEngine& ge, const GameState& gs) {
    auto variants = ge.generateActions(gs);

    return variants[rng() % variants.size()];
}