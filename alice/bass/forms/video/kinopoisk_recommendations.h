#pragma once

#include "defs.h"
#include "video_slots.h"

#include <alice/bass/libs/globalctx/fwd.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/video_common/kinopoisk_recommendations.h>

#include <alice/library/util/min_heap.h>
#include <alice/library/util/rng.h>


#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/generic/is_in.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/generic/vector.h>
#include <util/system/rwlock.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace NBASS {
namespace NVideo {

class TKinopoiskRecommendations : private NNonCopyable::TNonCopyable {
public:
    struct TEntry {
        TEntry() = default;

        TEntry(size_t index, double weight)
            : Index(index)
            , Weight(weight) {
        }

        bool operator<(const TEntry& rhs) const {
            return Weight < rhs.Weight;
        }

        bool operator>(const TEntry& rhs) const {
            return rhs < *this;
        }

        size_t Index = 0;
        double Weight = 0;
    };

    struct TData {
        static inline constexpr double kMinRating = 1e-3;

        template <typename TInfos>
        explicit TData(TInfos&& infos)
            : Infos(std::forward<TInfos>(infos))
            , OrderByReleaseDate(Infos.size()) {
            Sort(Infos, [](const NVideoCommon::TKinopoiskFilmInfo& lhs, const NVideoCommon::TKinopoiskFilmInfo& rhs) {
                return lhs.Rating > rhs.Rating;
            });

            Iota(OrderByReleaseDate.begin(), OrderByReleaseDate.end(), static_cast<size_t>(0));
            Sort(OrderByReleaseDate, [&](size_t li, size_t ri) {
                Y_ASSERT(li < Infos.size());
                Y_ASSERT(ri < Infos.size());
                const auto& lhs = Infos[li];
                const auto& rhs = Infos[ri];

                if (lhs.ReleaseDate.Defined() && !rhs.ReleaseDate.Defined())
                    return true;
                if (lhs.ReleaseDate.Defined() && rhs.ReleaseDate.Defined() && *lhs.ReleaseDate > *rhs.ReleaseDate)
                    return true;
                return false;
            });
        }

        template <typename TOnInfo>
        void RecommendRandom(size_t limit, TContentTypeFlags contentType,
                             const TMaybe<NVideoCommon::EVideoGenre>& genre, TOnInfo&& onInfo,
                             NAlice::IRng& rng) const {
            NAlice::TMinHeap<TEntry> top;

            for (size_t i = 0; i < Infos.size(); ++i) {
                const auto& info = Infos[i];
                if (!Matches(info, contentType, genre))
                    continue;

                const double r = Max(info.Rating, kMinRating);
                const double w = pow(rng.RandomDouble(), 1.0 / r);

                if (top.Size() < limit) {
                    top.Emplace(i, w);
                } else if (!top.Empty() && top.GetMin().Weight < w) {
                    top.ExtractMin();
                    top.Emplace(i, w);
                }
            }

            auto selected = top.Steal();
            Sort(selected, [](const TEntry& lhs, const TEntry& rhs) { return lhs.Index < rhs.Index; });

            for (const auto& entry : selected) {
                const auto& index = entry.Index;
                Y_ASSERT(index < Infos.size());
                onInfo(Infos[index]);
            }
        }

        template <typename TOnInfo>
        void RecommendTop(size_t limit, TContentTypeFlags contentType, const TMaybe<NVideoCommon::EVideoGenre>& genre,
                          TOnInfo&& onInfo) const {
            size_t recommended = 0;

            for (size_t i = 0; i < Infos.size() && recommended != limit; ++i) {
                const auto& info = Infos[i];
                if (!Matches(info, contentType, genre))
                    continue;

                onInfo(info);
                ++recommended;
            }
        }

        template <typename TOnInfo>
        void RecommendNew(size_t limit, TContentTypeFlags contentType, const TMaybe<NVideoCommon::EVideoGenre>& genre,
                          TOnInfo&& onInfo) const {
            Y_ASSERT(Infos.size() == OrderByReleaseDate.size());

            size_t recommended = 0;

            for (size_t i = 0; i < OrderByReleaseDate.size() && recommended != limit; ++i) {
                Y_ASSERT(OrderByReleaseDate[i] < Infos.size());
                const auto& info = Infos[OrderByReleaseDate[i]];

                if (!Matches(info, contentType, genre))
                    continue;

                onInfo(info);
                ++recommended;
            }
        }

        template <typename TOnInfo>
        size_t RecommendMultistep(size_t limit, TContentTypeFlags contentType,
                                  const TVideoSlots& slots, TOnInfo&& onInfo) const {
            size_t recommended = 0;
            size_t recommendableCount = 0;

            for (const auto& info : Infos) {
                if (!Matches(info, contentType, slots)) {
                    continue;
                }

                if (recommended != limit) {
                    onInfo(info);
                    ++recommended;
                }
                ++recommendableCount;
            }

            return recommendableCount;
        }

        static bool Matches(const NVideoCommon::TKinopoiskFilmInfo& info, TContentTypeFlags contentType,
                            const TVideoSlots& slots) {
            if (!(contentType & info.ContentType)) {
                return false;
            }

            if (slots.VideoGenre.Defined() && !IsIn(info.Genres, slots.VideoGenre.GetEnumValue())) {
                return false;
            }

            if (slots.Country.Defined() && !IsIn(info.Countries, slots.Country.GetString())) {
                return false;
            }

            if (!ReleaseDateMatches(info.ReleaseDate, slots.ReleaseDate)) {
                return false;
            }

            return true;
        }

        static bool ReleaseDateMatches(const TMaybe<NDatetime::TCivilDay>& itemReleaseDate,
                                       const TReleaseYearSlot& requestedReleaseYear) {
            if (!requestedReleaseYear.Defined()) {
                return true;
            }

            if (!itemReleaseDate) {
                return false;
            }

            if (requestedReleaseYear.ExactYear) {
                return itemReleaseDate->year() == *requestedReleaseYear.ExactYear;
            }

            if (requestedReleaseYear.DecadeStartYear) {
                const auto decadeStartYear = *requestedReleaseYear.DecadeStartYear;
                return decadeStartYear <= itemReleaseDate->year() && itemReleaseDate->year() < decadeStartYear + 10;
            }

            if (requestedReleaseYear.YearsRange) {
                const auto& [from, to] = *requestedReleaseYear.YearsRange;
                return from <= itemReleaseDate->year() && itemReleaseDate->year() <= to;
            }

            return false;
        }

        static bool Matches(const NVideoCommon::TKinopoiskFilmInfo& info, TContentTypeFlags contentType,
                            const TMaybe<NVideoCommon::EVideoGenre>& genre) {
            if (!(contentType & info.ContentType))
                return false;

            const auto& genres = info.Genres;
            if (genre && Find(genres, *genre) == genres.end())
                return false;

            return true;
        }

        TVector<NVideoCommon::TKinopoiskFilmInfo> Infos;
        TVector<size_t> OrderByReleaseDate;
    };

public:
    static TKinopoiskRecommendations& Instance() {
        return *Singleton<TKinopoiskRecommendations>();
    }

    TDuration Update(TGlobalContextPtr ctx);
    TAtomicSharedPtr<TData> GetData() const;

    void SetUpdatePeriod(const TDuration& updatePeriod);
    TDuration GetUpdatePeriod();

private:
    Y_DECLARE_SINGLETON_FRIEND();

    TKinopoiskRecommendations() = default;

private:
    TAtomicSharedPtr<TData> Data;
    TDuration UpdatePeriod;
    TRWMutex Mutex;
};

} // namespace NVideo
} // namespace NBASS
