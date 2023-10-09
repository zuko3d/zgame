#include "Tournament.h"

#include "GameEngine.h"
#include "Timer.h"

#include <fstream>
#include <iostream>


GameResult Tournament::playSingleGame(const GameRules& rules, const std::vector<IBot*>& bots, int seed, bool withLogs) {
    GameEngine ge(rules, bots);
    const auto winPoints = ge.playGame(seed);

    return GameResult{
		-1, // FIX ME!!!
		winPoints,
		ge.getStatsCopy(),
		{-1}, // FIX ME!!!
	};
}

std::vector<GameResult> Tournament::playAllInAll(const GameRules& rules, const std::vector<IBot*>& bots, int repeat, int timeLimit) {
	std::vector<GameResult> results;

#ifndef DEBUG
#pragma omp parallel for schedule(dynamic)
#endif
	for (int p0 = 0; p0 < bots.size(); p0++) {
		Timer timer;
		for (int seed = 0; seed < repeat; seed++) {
			// std::cout << ".";
			// std::cout.flush();
			for (int p1 = p0 + 1; p1 < bots.size(); p1++) {
				{
					GameEngine ge(rules, {bots.at(p0), bots.at(p1)});
					const auto res = ge.playGame(seed, false, true);
#pragma omp critical
					results.push_back(
						GameResult{
							(int)std::distance(res.begin(), std::max_element(res.begin(), res.end())),
							res,
							ge.getStatsCopy(),
							{p0, p1},
						}
					);

				}
				{
					GameEngine ge(rules, {bots.at(p1), bots.at(p0)});
					const auto res = ge.playGame(seed, false, true);
#pragma omp critical
					results.push_back(
						GameResult{
							(int)std::distance(res.begin(), std::max_element(res.begin(), res.end())),
							res,
							ge.getStatsCopy(),
							{p1, p0},
						}
					);
				}
			}
			if (timer.elapsedSeconds() > timeLimit) {
				std::cout << "!";
				std::cout.flush();
				break;
			}
		}
		std::cout << p0 << " ";
		std::cout.flush();
	}

	// std::cout << std::endl;
	return results;
}

std::pair<std::vector<double>, std::vector<double>> Tournament::playTeamVTeam(const GameRules& rules, const std::vector<IBot*>& bots1, const std::vector<IBot*>& bots2, int repeat) {
	std::vector<double> wins(bots1.size(), 0.0);
	std::vector<double> games(bots1.size(), 0.0);
	std::vector<double> winPoints(bots1.size(), 0.0);

#ifndef DEBUG
#pragma omp parallel for schedule(dynamic)
#endif
	for (int p0 = 0; p0 < bots1.size(); p0++) {
		Timer timer;
		for (int seed = 0; seed < repeat; seed++) {
			// std::cout << ".";
			// std::cout.flush();
			for (int p1 = 0; p1 < bots2.size(); p1++) {
				games.at(p0) += 2;
				// games.at(p1) += 2;

				auto res = playSingleGame(rules, {bots1.at(p0), bots2.at(p1)}, seed);
				winPoints.at(p0) += res.winPoints.at(0);
				// winPoints.at(p1) += res.winPoints.at(1);
				if (res.winner == 0) {
					wins.at(p0)++;
				} else {
					// wins.at(p1)++;
				}

				res = playSingleGame(rules, {bots2.at(p1), bots1.at(p0)}, seed);
				winPoints.at(p0) += res.winPoints.at(1);
				// winPoints.at(p1) += res.winPoints.at(0);
				if (res.winner == 0) {
					// wins.at(p1)++;
				} else {
					wins.at(p0)++;
				}
				if (timer.elapsedSeconds() > 20) {
					break;
				}
			}
			if (timer.elapsedSeconds() > 20) {
				std::cout << "!";
				break;
			}
		}
	}

	// std::cout << std::endl;

	std::vector<double> winrates;
	for (int i = 0; i < wins.size(); i++) {
		winrates.push_back(wins.at(i) / games.at(i));
		winPoints.at(i) /= games.at(i);
	}

	return {
		std::move(winrates),
		std::move(winPoints)
	};
}

std::vector<GameResult> Tournament::playSP(const EngineTrainParams& etp, const GameRules& rules, const std::vector<IBot*>& bots, int repeat, int seedStart, bool withStats, int timeLimitSeconds) {
	std::vector<GameResult> results;

	static std::unordered_map<std::string, std::vector<GameResult>> cache;

#ifndef DEBUG
#pragma omp parallel for schedule(dynamic)
#endif
	for (int p0 = 0; p0 < bots.size(); p0++) {
		Timer timer;

		std::string hash;
		hash += bots.at(p0)->hashState() + "#";
		hash += std::to_string(rules.seed) + "$";
		hash += std::to_string(repeat) + "$";
		hash += std::to_string(seedStart) + "$";
		if (cache.contains(hash)) {
			// std::cout << "-- CACHED! --" << std::endl;
			auto cached = cache.at(hash);
			for (auto& gr: cached) {
				gr.botIndices = {p0};
			}
#pragma omp critical (res_insertion)
			std::copy(cached.begin(), cached.end(), std::back_inserter(results));
			continue;
		}

		std::vector<GameResult> botResults;
		for (int seed = seedStart; seed < seedStart + repeat; seed++) {
			GameEngine ge(rules, {bots.at(p0)}, etp);
			const auto res = ge.playGame(seed, false, withStats);
			botResults.push_back(
				GameResult{
					0,
					res,
					ge.getStatsCopy(),
					{p0},
				}
			);
			
			if (timer.elapsedSeconds() > timeLimitSeconds) {
				break;
			}
		}
		// cache.emplace(hash, botResults);
#pragma omp critical (res_insertion)
		std::copy(botResults.begin(), botResults.end(), std::back_inserter(results));
	}

	return results;
}

ActionStats Tournament::measureRules(const GameRules& rules, const std::vector<IBot*>& bots, int repeat, int seedStart) {
	//     // aka balance. No action diversity means there is only one strategy
    // double actionDiversity;
    // double wpSourceDiversity;

    // // how much mcts steps do you need to beat 100-steps and 1k-steps with 60% (80%?) winrate?
    // // less = better
    // double skillIntensity;
    // int skillCapInMctsSteps;
    // // double avgMctsComplexity;

    // // more = better, different strategies = good
    // double botsWinrateDispersion;
	RulesMetrics ret;
	std::ofstream fout("rules/rules_v2_tune_" + std::to_string(rules.tuneVersion) + "_" + std::to_string(rules.seed) + ".html");
	fout << "<html><body>";

	// calibrate and get only strong ones
	std::vector<IBot*> strongBots;
	{
		const auto basicResults = playAllInAll(rules, bots, repeat, 600);
		const auto winrates = GameResult::winrates(basicResults);
		for (const auto& [wr, idx]: enumerate(winrates)) {
			std::cout << idx << ": " << wr << std::endl;
			if (wr >= 0.4) {
				strongBots.push_back(bots.at(idx));
			}
		}
	}
	ret.botElitism = ((double) strongBots.size()) / bots.size();
	std::cout << "ret.botElitism: " << ret.botElitism <<std::endl;

	const auto results = playAllInAll(rules, strongBots, repeat, 600);
	std::vector<std::vector<int>> games(strongBots.size(), std::vector<int>(strongBots.size(), 0));
	std::vector<std::vector<int>> wins(games);

	double p1wins = 0;
	for (const auto& r: results) {
		if (r.winner == 0) {
			p1wins++;
		}
		games.at(r.botIndices.at(0)).at(r.botIndices.at(1))++;
		games.at(r.botIndices.at(1)).at(r.botIndices.at(0))++;
		if (r.winner == 0) {
			wins.at(r.botIndices.at(0)).at(r.botIndices.at(1))++;
		} else {
			wins.at(r.botIndices.at(1)).at(r.botIndices.at(0))++;
		}
	}
	ret.p1p2winrate = p1wins / results.size();
	std::cout << "ret.p1p2winrate: " << ret.p1p2winrate << " of " << results.size() << " games" << std::endl;
	fout << "p1p2winrate: " << ret.p1p2winrate << "<br>";

	const auto actionStats = ActionStats(results);
	fout << actionStats.serialize() << "<br>";
	std::cout << actionStats.serialize() << std::endl;

	auto strongWinpoints = GameResult::avgWinPoints(results);
	std::sort(strongWinpoints.begin(), strongWinpoints.end());
	fout << "strongWinpoints: " << strongWinpoints.front() << " - " << strongWinpoints.back() << "<br>";
	std::cout << "strongWinpoints: " << strongWinpoints.front() << " - " << strongWinpoints.back() << std::endl;

	std::vector<std::vector<double>> winrates(strongBots.size());
	std::vector<std::vector<double>> winratesPositional(strongBots.size(), std::vector<double>(strongBots.size()));
	for (int p1 = 0; p1 < strongBots.size(); p1++) {
		for (int p2 = 0; p2 < strongBots.size(); p2++) {
			winratesPositional.at(p1).at(p2) = ((double) wins.at(p1).at(p2)) / games.at(p1).at(p2);
			if (p1 == p2) continue;
			winrates.at(p1).push_back(((double) wins.at(p1).at(p2)) / games.at(p1).at(p2));
		}
	}

	for (double th: {0.48, 0.55, 0.6}) {
		double transitive = 0.0;
		double total = 0.0;
		for (int p1 = 0; p1 < winrates.size(); p1++) {
			for (int p2 = p1 + 1; p2 < winrates.size(); p2++) {
				for (int p3 = p2 + 1; p3 < winrates.size(); p3++) {
					total++;
					if (winratesPositional.at(p1).at(p2) > th &&
							winratesPositional.at(p2).at(p3) > th &&
							winratesPositional.at(p3).at(p1) > th) {
						transitive++;
					}
				}
			}
		}
		std::cout << "nontransitivity " << th << ": " << (transitive / total) << std::endl;
		fout << "nontransitivity: " << th << ": " << (transitive / total) << "<br>";
	}
	{
		double total = 0.0;
		ret.matchupNonTransitivity = 0.0;
		for (int p1 = 0; p1 < winrates.size(); p1++) {
			for (int p2 = p1 + 1; p2 < winrates.size(); p2++) {
				for (int p3 = p2 + 1; p3 < winrates.size(); p3++) {
					total++;
					ret.matchupNonTransitivity += fabs(-1.5 + winratesPositional.at(p1).at(p2) +
						winratesPositional.at(p2).at(p3) +
						winratesPositional.at(p3).at(p1));
				}
			}
		}
		ret.matchupNonTransitivity = ret.matchupNonTransitivity / total;
		std::cout << "ret.matchupNonTransitivity: " << ret.matchupNonTransitivity << std::endl;
		fout << "matchupNonTransitivity: " << ret.matchupNonTransitivity << "<br>";
	}

	std::vector<double> botsWinrateDispersion;
	std::cout << "botsWinrateDispersion: \n";
	for (const auto& [wr, idx]: enumerate(winrates)) {
		std::cout << "mean: " << mean(wr) << "\t var: " << variance(wr) << "\t total: " << sum(games.at(idx)) << std::endl;
		botsWinrateDispersion.push_back(variance(wr));
	}
	ret.botsWinrateDispersion = mean(botsWinrateDispersion);
	std::cout << "ret.botsWinrateDispersion = " << ret.botsWinrateDispersion << std::endl;
	fout << "botsWinrateDispersion: " << ret.botsWinrateDispersion << "<br>";

	// double prevMeanWps = 0.0;
	
	// double meanWps = 0.0;
	// for (auto& bot: strongBots) {
	// 	bot->resetSkill();
	// }
	// if (strongBots.size() > 2) {
	// 	for (int sk = 0; sk < 7; sk++) {
	// 		for (auto& bot: strongBots) {
	// 			bot->upgradeSkill();
	// 		}
	// 		const auto results = Tournament::playSP(EngineTrainParams{}, rules, strongBots, 60, 1010101, false, 600);
	// 		auto winPoints = GameResult::avgWinPoints(results);
	// 		// prevMeanWps = meanWps;
	// 		std::sort(winPoints.begin(), winPoints.end());
	// 		meanWps = winPoints.at(1);
	// 		std::cout << "skill " << strongBots.front()->getSkill() << "\t wp: " << meanWps 
	// 			<< " | " << mean(winPoints) << " | " << winPoints.at(winPoints.size() - 2) << std::endl; 
	// 	}
	// 	//  while (meanWps > prevMeanWps * 1.04);
	// 	ret.skillCapInMctsSteps = strongBots.front()->getSkill();
	// 	std::cout << "ret.skillCapInMctsSteps: " << ret.skillCapInMctsSteps << std::endl;
	// }

	fout << "<br><br><br>";
	fout << rules.renderRulesHtml();
	fout << "</body></html>";

	return actionStats;
}
