#ifndef MINION_SEARCHSTRATEGIES_H
#define MINION_SEARCHSTRATEGIES_H

#include "NeighbourhoodChoosingStrategies.h"
#include "neighbourhood-search.h"
#include "search/neighbourhood-search.h"
#include "search/nhConfig.h"
#include <algorithm>
#include <cmath>

template <typename Integer>
class ExponentialIncrementer {
  double value;
  const double multiplier;
  const double increment;

public:
  ExponentialIncrementer(double initialValue, double multiplier, double increment)
      : value(initialValue), multiplier(multiplier), increment(increment) {}
  void increase() {
    value *= multiplier;
    value += increment;
  }

  Integer getValue() {
    return std::round(value);
  }
};

template <typename SelectionStrategy>
class HillClimbingSearch {

  SelectionStrategy selectionStrategy;

  pair<size_t, NeighbourhoodStats>
  runNeighbourhood(NeighbourhoodState& nhState, ExponentialIncrementer<int> backtrackLimit,
                   const vector<size_t>& highestNeighbourhoodSizes) {
    int combinationToActivate = selectionStrategy.getCombinationsToActivate(nhState);
    SearchParams params = SearchParams::neighbourhoodSearch(
        combinationToActivate, nhState.nhc, true, true, false,
        getOptions().nhConfig->iterationSearchTime, backtrackLimit.getValue(),
        getOptions().nhConfig->backtrackInsteadOfTimeLimit,
        highestNeighbourhoodSizes[combinationToActivate]);
    NeighbourhoodStats stats = nhState.searchNeighbourhoods(params);
    selectionStrategy.updateStats(combinationToActivate, stats);

    return make_pair(combinationToActivate, stats);
  }
  void handleBetterSolution(NeighbourhoodState& nhState, NeighbourhoodStats& stats,
                            int& iterationsSpentAtPeak, double& localMaxProbability,
                            vector<size_t> highestNeighbourhoodSizes) {
    iterationsSpentAtPeak = 0;
    localMaxProbability = getOptions().nhConfig->hillClimberInitialLocalMaxProbability;
    highestNeighbourhoodSizes.assign(nhState.nhc.neighbourhoodCombinations.size(), 1);
    bestSolutionValue = stats.newMinValue;
    bestSolution = std::move(nhState.solution);
    nhState.solution = {};
    nhState.copyOverIncumbent(bestSolution);
    getState().getOptimiseVar()->setMin(stats.newMinValue);
    nhState.propagate();
  }

public:
  DomainInt bestSolutionValue;
  std::vector<DomainInt> bestSolution;

  HillClimbingSearch(const NeighbourhoodContainer& nhc) : selectionStrategy(nhc) {}

  void run(NeighbourhoodState& nhState, DomainInt initSolutionValue,
           std::vector<DomainInt>& initSolution) {

    int iterationsSpentAtPeak = 0;
    size_t numberIterationsAtStart = nhState.globalStats.numberIterations;
    // The probability that we will enter a random mode of exploration.
    double localMaxProbability = getOptions().nhConfig->hillClimberInitialLocalMaxProbability;
    auto& nhConfig = getOptions().nhConfig;
    ExponentialIncrementer<int> backtrackLimit(nhConfig->initialBacktrackLimit,
                                               nhConfig->backtrackLimitMultiplier,
                                               nhConfig->backtrackLimitIncrement);
    vector<size_t> highestNeighbourhoodSizes(nhState.nhc.neighbourhoodCombinations.size(), 1);

    bestSolutionValue = initSolutionValue;
    bestSolution = initSolution;
    nhState.copyOverIncumbent(bestSolution);
    getState().getOptimiseVar()->setMin(bestSolutionValue);
    nhState.propagate();
    while(true) {
      auto nhInfo = runNeighbourhood(nhState, backtrackLimit, highestNeighbourhoodSizes);
      NeighbourhoodStats& stats = nhInfo.second;
      if(!getOptions().nhConfig->increaseBacktrackOnlyOnFailure || !stats.solutionFound) {
        backtrackLimit.increase();
      }
      if(stats.solutionFound) {
        handleBetterSolution(nhState, stats, iterationsSpentAtPeak, localMaxProbability,
                             highestNeighbourhoodSizes);
      } else {
        highestNeighbourhoodSizes[nhInfo.first] =
            checked_cast<size_t>(stats.highestNeighbourhoodSize);
        localMaxProbability += (1.0 / nhState.nhc.neighbourhoodCombinations.size()) *
                               getOptions().nhConfig->hillClimberProbabilityIncrementMultiplier;
        ++iterationsSpentAtPeak;
        if(iterationsSpentAtPeak > getOptions().nhConfig->hillClimberMinIterationsToSpendAtPeak &&
           static_cast<double>(std::rand()) / RAND_MAX < localMaxProbability) {
          nhState.globalStats.notifyEndClimb();
          cout << "numberIterations: "
               << (nhState.globalStats.numberIterations - numberIterationsAtStart) << std::endl;
          return;
        }
      }
    }
  }
};

template <typename SelectionStrategy>
class LateAcceptanceHillClimbingSearch : public HillClimbingSearch<SelectionStrategy> {
public:
  LateAcceptanceHillClimbingSearch(const NeighbourhoodContainer& nhc)
      : HillClimbingSearch<SelectionStrategy>(nhc) {
    std::cout << "todo\n";
    abort();
  }
};
template <typename SelectionStrategy>
class SimulatedAnnealingSearch : public HillClimbingSearch<SelectionStrategy> {
public:
  SimulatedAnnealingSearch(const NeighbourhoodContainer& nhc)
      : HillClimbingSearch<SelectionStrategy>(nhc) {
    std::cout << "todo\n";
    abort();
  }
};
template <typename SearchStrategy>
class MetaSearch {
  SearchStrategy searchStrategy;
  int minNeighbourhoodSize = 0;
  int neighbourhoodSizeOffset = 0;
  ExponentialIncrementer<int> backtrackLimit;

public:
  std::vector<DomainInt> bestSolution;
  DomainInt bestSolutionValue;
  MetaSearch(const NeighbourhoodContainer& nhc)
      : searchStrategy(nhc),
        backtrackLimit(getOptions().nhConfig->holePuncherInitialBacktrackLimit,
                       getOptions().nhConfig->holePuncherBacktrackLimitMultiplier, 0) {}

  void run(NeighbourhoodState& nhState, DomainInt initSolutionValue,
           std::vector<DomainInt>& initSolution) {
    searchStrategy.run(nhState, initSolutionValue, initSolution);
    bestSolution = std::move(searchStrategy.bestSolution);
    bestSolutionValue = searchStrategy.bestSolutionValue;
    resetNeighbourhoodSize();
    while(true) {
      bool success = false;
      auto availableNHCombinations = findNextNeighbourhoodSizeWithActiveCombinations(nhState.nhc);
      if(availableNHCombinations.empty()) {
        randomClimbUntilBetter(nhState);
        resetNeighbourhoodSize();
        continue;
      }
      for(int nhIndex : availableNHCombinations) {
        NeighbourhoodStats stats = runNeighbourhood(nhState, nhIndex);
        searchStrategy.run(nhState, stats.newMinValue, nhState.solution);
        if(searchStrategy.bestSolutionValue > bestSolutionValue) {
          bestSolutionValue = searchStrategy.bestSolutionValue;
          bestSolution = searchStrategy.bestSolution;
          resetNeighbourhoodSize();
          success = true;
          break;
        }
      }
      if(!success) {
        minNeighbourhoodSize *= 2;
      }
    }
  }

  void randomClimbUntilBetter(NeighbourhoodState& nhState) {
    while(true) {
      NeighbourhoodStats stats = findRandomSolutionUsingNormalSearch(nhState);
      if(stats.newMinValue > bestSolutionValue) {
        bestSolutionValue = stats.newMinValue;
        bestSolution = nhState.solution;
        return;
      }
    }
  }

  NeighbourhoodStats runNeighbourhood(NeighbourhoodState& nhState, size_t nhIndex) {
    SearchParams params = SearchParams::neighbourhoodSearch(
        nhIndex, nhState.nhc, true, false, true, getOptions().nhConfig->iterationSearchTime,
        backtrackLimit.getValue(), getOptions().nhConfig->backtrackInsteadOfTimeLimit,
        currentNeighbourhoodSize());
    NeighbourhoodStats stats = nhState.searchNeighbourhoods(params);
    if(!stats.solutionFound) {
      backtrackLimit.increase();
    }
    return stats;
  }
  void resetNeighbourhoodSize() {
    minNeighbourhoodSize = 1;
    neighbourhoodSizeOffset = 0;
  }
  inline int currentNeighbourhoodSize() const {
    return minNeighbourhoodSize + neighbourhoodSizeOffset;
  }

  inline std::vector<int>
  findNextNeighbourhoodSizeWithActiveCombinations(const NeighbourhoodContainer& nhc) {
    std::vector<int> activeCombinations;
    int maxNHSize = nhc.getMaxNeighbourhoodSize();
    while(currentNeighbourhoodSize() <= maxNHSize) {
      activeCombinations.clear();
      for(int i = 0; i < nhc.neighbourhoodCombinations.size(); ++i) {
        if(nhc.isCombinationEnabled(i) &&
           nhc.neighbourhoods[nhc.neighbourhoodCombinations[i][0]].deviation.inDomain(
               currentNeighbourhoodSize()))
          activeCombinations.push_back(i);
      }
      if(!activeCombinations.empty()) {
        break;
      } else {
        ++neighbourhoodSizeOffset;
      }
    }
    if(!activeCombinations.empty()) {
      std::random_shuffle(activeCombinations.begin(), activeCombinations.end());
    }
    return activeCombinations;
  }
};

template <typename NhSelectionStrategy>
shared_ptr<Controller::SearchManager> MakeNeighbourhoodSearchHelper(PropagationLevel& prop_method,
                                                                    vector<SearchOrder>& base_order,
                                                                    NeighbourhoodContainer& nhc) {
  shared_ptr<Propagate> prop = Controller::make_propagator(prop_method);
  switch(getOptions().neighbourhoodSearchStrategy) {
  case SearchOptions::NeighbourhoodSearchStrategy::META_WITH_HILLCLIMBING:
    return std::make_shared<
        NeighbourhoodSearchManager<MetaSearch<HillClimbingSearch<NhSelectionStrategy>>>>(
        prop, base_order, nhc);
  case SearchOptions::NeighbourhoodSearchStrategy::META_WITH_LAHC:
    return std::make_shared<NeighbourhoodSearchManager<
        MetaSearch<LateAcceptanceHillClimbingSearch<NhSelectionStrategy>>>>(prop, base_order, nhc);
  case SearchOptions::NeighbourhoodSearchStrategy::META_WITH_SIMULATED_ANEALING:
    return std::make_shared<
        NeighbourhoodSearchManager<MetaSearch<SimulatedAnnealingSearch<NhSelectionStrategy>>>>(
        prop, base_order, nhc);
  case SearchOptions::NeighbourhoodSearchStrategy::HILL_CLIMBING:
    return std::make_shared<NeighbourhoodSearchManager<HillClimbingSearch<NhSelectionStrategy>>>(
        prop, base_order, std::move(nhc));
  case SearchOptions::NeighbourhoodSearchStrategy::LAHC:
    return std::make_shared<
        NeighbourhoodSearchManager<LateAcceptanceHillClimbingSearch<NhSelectionStrategy>>>(
        prop, base_order, nhc);
  case SearchOptions::NeighbourhoodSearchStrategy::SIMULATED_ANEALING:
    return std::make_shared<
        NeighbourhoodSearchManager<SimulatedAnnealingSearch<NhSelectionStrategy>>>(prop, base_order,
                                                                                   nhc);
  default: assert(false); abort();
  }
}

shared_ptr<Controller::SearchManager> MakeNeighbourhoodSearch(PropagationLevel prop_method,
                                                              vector<SearchOrder> base_order,
                                                              NeighbourhoodContainer nhc) {
  switch(getOptions().neighbourhoodSelectionStrategy) {
  case SearchOptions::NeighbourhoodSelectionStrategy::RANDOM:
    return MakeNeighbourhoodSearchHelper<RandomCombinationChooser>(prop_method, base_order, nhc);
  case SearchOptions::NeighbourhoodSelectionStrategy::UCB:
    return MakeNeighbourhoodSearchHelper<UCBNeighbourhoodSelection>(prop_method, base_order, nhc);
  case SearchOptions::NeighbourhoodSelectionStrategy::LEARNING_AUTOMATON:
    return MakeNeighbourhoodSearchHelper<LearningAutomatonNeighbourhoodSelection>(prop_method,
                                                                                  base_order, nhc);
  case SearchOptions::NeighbourhoodSelectionStrategy::INTERACTIVE:
    return MakeNeighbourhoodSearchHelper<InteractiveCombinationChooser>(prop_method, base_order,
                                                                        nhc);
  default: assert(false); abort();
  }
}

#endif
