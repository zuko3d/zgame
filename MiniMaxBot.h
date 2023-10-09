// #pragma once

// #include "MctsBot.h"

// class MiniMaxBot : public MctsBot {
// public:
//     MiniMaxBot(MctsBotParams botParams)
//         : MctsBot(botParams)
//     { }

//     MiniMaxBot(IBot* oppBot_, int steps_, int mctsTimeLimitMilliseconds_, MctsBotParams botParams)
//         : MctsBot(oppBot_, steps_, mctsTimeLimitMilliseconds_, botParams)
//     { }

//     Action chooseAction(const GameEngine& ge, const GameState& gs);

//     virtual ~MiniMaxBot() { }

// protected:
//     virtual double measureGs(const GameEngine& ge, const GameState& gs) const;
// };