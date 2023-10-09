#pragma once

#include "IBot.h"
#include "GameEngine.h"
#include "GameCounters.h"

#include <string>
#include <sstream>
#include <random>
#include <vector>

struct MctsNode;

struct Mutation {
    int paramIdx = -1;
    double change = 1.0;
};

struct MctsBotParams {
    static MctsBotParams generate(std::default_random_engine& rng) {
        return MctsBotParams {
            (int) (urand(rng, 4.0, 25.0)),// maxDepth
            lnrand(rng) * 1.41,     // C
        };
    }

    std::vector<std::pair<MctsBotParams, Mutation>> genMutations(double magnitude = 0.1) const {
        std::vector<std::pair<MctsBotParams, Mutation>> ret;
        for (int pos = 0; pos < 2; pos++) {
            MctsBotParams newParams{*this};
            newParams.applyMutation(pos, exp(magnitude));
            ret.emplace_back(newParams, Mutation{pos, exp(magnitude)});
        }

        return ret;
    }

    void applyMutation(int pos, double magnitude = 1.0) {
        if (pos == 0) maxDepth *= magnitude;
        if (pos == 1) C *= magnitude;
    }

    void mutate(std::default_random_engine& rng, int amount, double magnitude = 0.1) {
        for (int i = 0; i < amount; i++) {
            applyMutation((int) urand(rng, 0.0, 2.0), exp(nrand(rng) * magnitude));
        }
    }

    void crossover(std::default_random_engine& rng, const MctsBotParams& op2) {
        if (rng() % 2) maxDepth = op2.maxDepth;
        if (rng() % 2) C = op2.C;
    }

    std::string serialize() const {
        std::ostringstream ss;
        ss << maxDepth << " ";
        ss << C << " ";

        return ss.str();
    }

    static MctsBotParams parse(std::string str) {
        std::istringstream ss(str);
        MctsBotParams ret;
        ss >> ret.maxDepth;
        ss >> ret.C;

        return ret;
    }

    int maxDepth;
    double C = -123.0;
};

class MctsBot : public IBot {
public:
    MctsBot(IBot* oppBot_, const MctsBotParams& botParams, std::vector<MeasurerParams> measurerParamsByRound_)
        : IBot(MeasurerParams{})
        , oppBot(oppBot_)
        , steps(256)
        , mctsTimeLimitMilliseconds(steps)
        , params(botParams)
        , depthStats(50)
        , measurerParamsByRound(std::move(measurerParamsByRound_))
    {
        assert(params.maxDepth >= 3);
    }

    MctsBot(const MctsBotParams& botParams, std::vector<MeasurerParams> measurerParamsByRound_)
        : IBot(MeasurerParams{})
        , oppBot(nullptr)
        , steps(256)
        , mctsTimeLimitMilliseconds(steps)
        , params(botParams)
        , depthStats(50)
        , measurerParamsByRound(std::move(measurerParamsByRound_))
    {
        assert(params.maxDepth >= 3);
    }

    MctsBot(IBot* oppBot_, int steps_, int mctsTimeLimitMilliseconds_, const MctsBotParams& botParams, std::vector<MeasurerParams> measurerParamsByRound_)
        : IBot(MeasurerParams{})
        , oppBot(oppBot_)
        , steps(steps_)
        , mctsTimeLimitMilliseconds(mctsTimeLimitMilliseconds_)
        , params(botParams)
        , depthStats(50)
        , measurerParamsByRound(std::move(measurerParamsByRound_))
    {
        assert(params.maxDepth >= 3);
    }

    Action chooseAction(const GameEngine& ge, const GameState& gs);

    // virtual Resources chooseResourcesForMarket(const GameEngine& ge, const GameState& gs, int param) const;
    virtual size_t chooseStartingWorkerCard(const GameEngine& ge, const GameState& gs, int param = 0) const;

    virtual std::string hashState() { return measurerParams.serialize() + " " + params.serialize() + " " + std::to_string(steps); }
    virtual void upgradeSkill() { steps *= 4; mctsTimeLimitMilliseconds = steps * 10; };
    virtual void resetSkill() { steps = 1; mctsTimeLimitMilliseconds = steps * 10; };
    virtual int getSkill() { return steps; };

    virtual void printInternalStats() const;

    virtual double measureGs(const GameCounters& gc) const;

    virtual ~MctsBot() { }

protected:
    void genChildren(MctsNode& node, const GameState& gs, const GameEngine& ge) const;
    MctsNode* goBottom(MctsNode& node, const GameState& gs, const GameEngine& ge) const;
    const MctsNode* buildMcTree(MctsNode& root, const GameState& gs, const GameEngine& ge) const;
    void advanceToNextNode(const Action& action, GameState& gs, const GameEngine& ge) const;
    // const GameEngine& ge;
    IBot* oppBot = nullptr;
    int steps;
    int mctsTimeLimitMilliseconds = 200;
    MctsBotParams params;
    mutable std::default_random_engine rng;

    mutable std::vector<std::vector<int> > depthStats;
    std::vector<MeasurerParams> measurerParamsByRound;
};

std::pair<std::vector<MctsBot>, std::vector<IBot*>> toPtrVector(const std::vector<MctsBotParams>& botParams, const std::vector<std::vector<MeasurerParams>>& paramsByRound);
