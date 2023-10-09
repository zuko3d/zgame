#pragma once

enum class TechEffect {
    MarketWorker,
    OppWorker,
    AdditionalResourceWorker,
    PolyWorker,

    AdditionalResourceOnMarketSell, // gold, wp, ...

    AdditionalResourceBuildingByColor,
    NoWorkerUpkeep,
    NoBuildingUpkeep,
    MoreWorkersInBuilding,
    WpPerBuildingColor,
    WpPerWorkerGrade,
    ResourceIncomeEveryTurn,
    ResourceIncomeEveryUpkeep,

    BuildingPriceDiscount,

    BuildForFree,
    BuildForCost,
    GetWorkerCardsForFree,
    GetWorkerCardsForCost,
};

enum TechFilter {
    ByColorOrGrade,

};

class Tech {
    int tier;

};