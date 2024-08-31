#include "order_algorithm.h"

#include <util/generic/algorithm.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywoodFw::NOrder {

TVector<TSet<TString>> FindUniqueElements(const TVector<TSet<TString>>& uniqueItemsOfOrders) {
    TVector<TSet<TString>> finalItemsOfOrders = uniqueItemsOfOrders;
    for (size_t i = 0; i < uniqueItemsOfOrders.size() - 1; i++) {
        for (size_t j = i + 1; j < uniqueItemsOfOrders.size(); j++) {
            TSet<TString> differenceIJResult;

            SetDifference(finalItemsOfOrders[i].begin(), finalItemsOfOrders[i].end(), 
                        uniqueItemsOfOrders[j].begin(), uniqueItemsOfOrders[j].end(), 
                        std::inserter(differenceIJResult, differenceIJResult.begin()));

            TSet<TString> differenceJIResult;

            SetDifference(finalItemsOfOrders[j].begin(), finalItemsOfOrders[j].end(), 
                        uniqueItemsOfOrders[i].begin(), uniqueItemsOfOrders[i].end(), 
                        std::inserter(differenceJIResult, differenceJIResult.begin()));

            finalItemsOfOrders[i] = std::move(differenceIJResult);
            finalItemsOfOrders[j] = std::move(differenceJIResult);
        }
    }

    return finalItemsOfOrders;
}

}