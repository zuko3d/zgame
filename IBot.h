#pragma once

// #include "PlayerState.h"
// #include "GameState.h"

#include "Resources.h"
#include "Types.h"

#include <vector>
#include <iostream>

class GameEngine;
struct GameState;
struct Action;
class GameCounters;
// class MeasurerParams;
// class PlayerState;

double measureGs(const GameCounters& gc, const MeasurerParams& params);
double measureGs(const std::array<double, WpSourcesTotal>& sourceWeights, const GameCounters& gc, const MeasurerParams& params);

class IBot {
public:
    IBot(const MeasurerParams& params_)
        : measurerParams(params_)
    { }

    virtual Resources chooseWorkersToDeploy(const GameEngine& ge, const GameState& gs, bool fast = false) const;

    // assume we'll use all workers
    virtual Resources chooseResourcesForMarket(const GameEngine& ge, const GameState& gs, int param = 0) const;

    virtual size_t chooseStartingWorkerCard(const GameEngine& ge, const GameState& gs, int param = 0) const;

    virtual Action chooseAction(const GameEngine& ge, const GameState& gs) = 0;
    
    virtual double measureGs(const GameCounters& gc) const;
    double measureGs(const std::array<double, WpSourcesTotal>& sourceWeights, const GameCounters& gc) const {
        return ::measureGs(sourceWeights, gc, measurerParams);
    }

    virtual void upgradeSkill() { };
    virtual void resetSkill() { };
    virtual int getSkill() { return 0; };
    virtual void printInternalStats() const { std::cout << "No stats!" << std::endl; };

    virtual std::string hashState() { return measurerParams.serialize(); }

    virtual ~IBot() { }

protected:
    MeasurerParams measurerParams;
};
