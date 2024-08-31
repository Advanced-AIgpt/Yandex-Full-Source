#include "items.h"

namespace NBASS {

TGalleryCandidateSelector ChooseBestCandidateForSeasonGallery(const TVector <TResolvedItem> &resolvedItems,
                                                              const NVideo::IContentInfoDelegate &ageChecker) {
    TGalleryCandidateSelector visitor(ageChecker);
    for (const auto &resolvedItem : resolvedItems)
        std::visit(visitor, resolvedItem.CandidateToPlay);

    return visitor;
}

TCandidatesToPlayCollector GroupCandidatesToPlay(const TVector <TResolvedItem> &resolvedItems,
                                                 const NVideo::IContentInfoDelegate &ageChecker,
                                                 TInstant requestStartTime) {
    TCandidatesToPlayCollector collector(ageChecker, requestStartTime);
    for (const auto &ri : resolvedItems)
        std::visit(collector, ri.CandidateToPlay);
    return collector;
}

} // namespace NBASS
