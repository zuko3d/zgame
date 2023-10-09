#include "BorrowBot.h"

#include "GameEngine.h"

 Action BorrowBot::chooseAction(const GameEngine& ge, const GameState& gs) {
    return {ActionType::Borrow, -1};
}