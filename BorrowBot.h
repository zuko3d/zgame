#pragma once

#include "IBot.h"

#include <random>

class BorrowBot : public IBot {
public:
    BorrowBot() 
        : IBot(MeasurerParams{})
    { }

    virtual Action chooseAction(const GameEngine& ge, const GameState& gs);
};
