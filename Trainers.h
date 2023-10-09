#pragma once

#include "GameRules.h"
#include "Types.h"
#include "ActionStats.h"

struct MctsBotParams;

#include <vector>

std::vector<MeasurerParams> trainParamsByRound(const GameRules& rules, const std::vector<MctsBotParams>& mctsBotParams);

void tuneRules(GameRules& rules, const ActionStats& actionStats, int seed);
