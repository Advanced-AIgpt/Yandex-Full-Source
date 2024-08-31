#pragma once

#include <alice/bass/forms/video/billing.h>
#include <alice/bass/forms/video/defs.h>
#include <alice/bass/forms/video/video_provider.h>

namespace NBASS {

struct TContextedItem {
    TContextedItem() = default;

    TContextedItem(const NSc::TValue &item, const NSc::TValue &tvShowItem)
            : Item(item), TvShowItem(tvShowItem) {
    }

    NVideo::TVideoItem Item;

    // This field has meaning only when Item is a tv-show episode.
    NVideo::TVideoItem TvShowItem;

    NSc::TValue ToJson() const {
        NSc::TValue serializedItem;
        serializedItem["item"] = Item.Value().Clone();
        serializedItem["tv_show_item"] = TvShowItem.Value().Clone();
        return serializedItem;
    }

    static TContextedItem FromJson(NSc::TValue value) {
        return TContextedItem(std::move(value["item"]), std::move(value["tv_show_item"]));
    }
};

struct TVideoClipsProviderResultSerializer {
    NSc::TValue operator()(const TError& err) const {
        NSc::TValue errValue;
        errValue["error"] = err.ToJson();
        return errValue;
    }

    NSc::TValue operator()(const NVideo::EBadArgument& arg) const {
        NSc::TValue result;
        result["bad_argument"] = ToString(arg);
        return result;
    }
};


struct TCandidateToPlay {
    struct TError {
        TError() = default;

        TError(const NVideo::TVideoItem &parent, const NVideo::TEpisodeIndex &index,
               NVideo::IVideoClipsProvider::TResult result)
                : Parent(parent), Index(index), Result(result) {
        }

        NVideo::TVideoItem Parent;
        NVideo::TEpisodeIndex Index;
        NVideo::IVideoClipsProvider::TResult Result;

        NSc::TValue ToJson() const {
            NSc::TValue serializedError;
            serializedError["parent"] = Parent.Value().Clone();

            NSc::TValue indexValue;
            indexValue["season"] = ToString(Index.Season);
            indexValue["episode"] = ToString(Index.Episode);
            serializedError["index"] = std::move(indexValue);

            if (Result.Defined())
                serializedError["result"] = std::visit(TVideoClipsProviderResultSerializer(), *Result);

            return serializedError;
        }

        static TMaybe<TError> FromJson(NSc::TValue value) {
            auto serialIndexFromString = [](TStringBuf source, NVideo::TSerialIndex &index) {
                ui32 indexInt;
                if (TryFromString(source, indexInt)) {
                    index = indexInt;
                    return true;
                }
                NVideo::ESpecialSerialNumber indexSpecial;
                if (TryFromString(source, indexSpecial)) {
                    index = indexSpecial;
                    return true;
                }
                return false;
            };

            NVideo::TEpisodeIndex epIndex;
            if (!serialIndexFromString(value["index"]["season"], epIndex.Season)) {
                LOG(ERR) << "Cannot parse season index from TValue: "
                         << value["index"]["season"] << Endl;
                return Nothing();
            }
            if (!serialIndexFromString(value["index"]["episode"], epIndex.Episode)) {
                LOG(ERR) << "Cannot parse episode index from TValue: "
                         << value["index"]["episode"] << Endl;
                return Nothing();
            }

            NVideo::TVideoItem item(std::move(value["parent"]));

            if (value["result"].IsNull())
                return TError(std::move(item), epIndex, Nothing());

            NSc::TValue resultValue = value["result"];
            NSc::TValue badArgValue = resultValue["bad_argument"];
            if (!badArgValue.IsNull()) {
                NVideo::EBadArgument ba;
                if (!TryFromString(std::move(badArgValue), ba)) {
                    LOG(WARNING) << "Cannot parse EBadArgument from TValue: "
                                 << badArgValue << Endl;
                    return Nothing();
                }
                return TError(std::move(item), epIndex, ba);
            }
            NSc::TValue errValue = resultValue["error"];
            if (!errValue.IsNull()) {
                return TError(std::move(item), epIndex,
                              NBASS::TError(std::move(errValue)));
            }

            return Nothing();
        }
    };

    TCandidateToPlay() = default;

    TCandidateToPlay(const NVideo::TVideoItem &curr, const NVideo::TVideoItem &next, const NVideo::TVideoItem &parent)
            : Curr(curr), Next(next), Parent(parent) {
    }

    NSc::TValue ToJson() const {
        NSc::TValue serializedCandidate;
        serializedCandidate["curr"] = Curr.Value().Clone();
        serializedCandidate["next"] = Next.Value().Clone();
        serializedCandidate["parent"] = Parent.Value().Clone();
        return serializedCandidate;
    }

    static TCandidateToPlay FromJson(NSc::TValue value) {
        return TCandidateToPlay(NVideo::TVideoItem(std::move(value["curr"])),
                                NVideo::TVideoItem(std::move(value["next"])),
                                NVideo::TVideoItem(std::move(value["parent"])));
    }

    NVideo::TVideoItem Curr;

    // Following fields have meaning only when Curr item is a tv-show
    // episode.
    NVideo::TVideoItem Next;
    NVideo::TVideoItem Parent;
};

struct TCandidateToPlayAndErrorSerializer {
    NSc::TValue operator()(const TCandidateToPlay &candidate) const {
        NSc::TValue result;
        result["candidate_to_play"] = candidate.ToJson();
        return result;
    }

    NSc::TValue operator()(const TCandidateToPlay::TError &err) const {
        NSc::TValue result;
        result["error"] = err.ToJson();
        return result;
    }
};

struct TResolvedItem {
    TResolvedItem() = default;

    TResolvedItem(const TContextedItem &contextedItem, const TCandidateToPlay &candidateToPlay)
            : ContextedItem(contextedItem), CandidateToPlay(candidateToPlay) {
    }

    TResolvedItem(const TContextedItem &contextedItem, const TCandidateToPlay::TError &error)
            : ContextedItem(contextedItem), CandidateToPlay(error) {
    }

    TContextedItem ContextedItem;
    std::variant<TCandidateToPlay, TCandidateToPlay::TError> CandidateToPlay;

    NSc::TValue ToJson() const {
        NSc::TValue serializedItem;
        serializedItem["contexted_item"] = ContextedItem.ToJson();
        serializedItem["candidate_to_play_or_err"] = std::visit(TCandidateToPlayAndErrorSerializer(), CandidateToPlay);
        return serializedItem;
    }

    static TMaybe<TResolvedItem> FromJson(NSc::TValue value) {
        TContextedItem ci = TContextedItem::FromJson(std::move(value["contexted_item"]));

        NSc::TValue candidateValue = value["candidate_to_play_or_err"]["candidate_to_play"];
        if (!candidateValue.IsNull())
            return TResolvedItem(std::move(ci), TCandidateToPlay::FromJson(std::move(candidateValue)));

        NSc::TValue errorValue = value["candidate_to_play_or_err"]["error"];
        if (!errorValue.IsNull()) {
            auto err = TCandidateToPlay::TError::FromJson(std::move(errorValue));
            if (!err.Defined())
                return Nothing();
            return TResolvedItem(std::move(ci), *err);
        }

        return Nothing();
    }
};

struct TCandidatesToPlayCollector {
    TCandidatesToPlayCollector(const NVideo::IContentInfoDelegate &ageChecker, TInstant requestStartTime)
            : AgeChecker(ageChecker), RequestStartTime(requestStartTime) {
    }

    void operator()(const TCandidateToPlay &candidate) {
        if (!AgeChecker.PassesAgeRestriction(candidate.Curr.Scheme()))
            HasCandidatesFailedAgeRestiction = true;
        else if (NVideo::IsItemNotIssued(candidate.Curr, RequestStartTime))
            HasCandidatesComingSoon = true;
        else if (NVideo::SupportedForBilling(candidate.Curr.Scheme()))
            SupportedByBilling[candidate.Curr->ProviderName()] = candidate;
        else
            FreeToPlay.push_back(candidate);
    }

    void operator()(const TCandidateToPlay::TError &error) {
        Errors.push_back(error);
    }

    THashMap<TString, TCandidateToPlay> SupportedByBilling;
    TVector<TCandidateToPlay> FreeToPlay;
    TVector<TCandidateToPlay::TError> Errors;
    bool HasCandidatesComingSoon = false;
    bool HasCandidatesFailedAgeRestiction = false;

    const NVideo::IContentInfoDelegate &AgeChecker;
    const TInstant RequestStartTime;
};

struct TGalleryCandidateSelector {
    TGalleryCandidateSelector(const NVideo::IContentInfoDelegate &ageChecker)
            : AgeChecker(ageChecker) {
    }

    void operator()(const TCandidateToPlay &candidate) {
        if (!AgeChecker.PassesAgeRestriction(candidate.Curr.Scheme())) {
            HasCandidatesFailedAgeRestiction = true;
        } else {
            ui32 count = candidate.Curr->SeasonsCount();
            if (!BestCount || *BestCount < count) {
                BestCount = count;
                BestIndex = CurrIndex;
            }
        }
        ++CurrIndex;
    }

    void operator()(const TCandidateToPlay::TError & /* error */) {
    }

    ui32 CurrIndex = 0;
    TMaybe<ui32> BestCount;
    TMaybe<ui32> BestIndex;
    bool HasCandidatesFailedAgeRestiction = false;
    const NVideo::IContentInfoDelegate &AgeChecker;
};


TGalleryCandidateSelector ChooseBestCandidateForSeasonGallery(const TVector<TResolvedItem> &resolvedItems,
                                                              const NVideo::IContentInfoDelegate &ageChecker);
TCandidatesToPlayCollector GroupCandidatesToPlay(const TVector<TResolvedItem> &resolvedItems,
                                                 const NVideo::IContentInfoDelegate &ageChecker,
                                                 TInstant requestStartTime);

} // namespace NBASS
