#pragma once

#include <alice/bass/forms/context/context.h>
#include <alice/bass/util/error.h>

#include <util/generic/map.h>
#include <util/stream/output.h>
#include <util/generic/vector.h>
#include <util/generic/strbuf.h>

namespace NBASS {

using TTokensWithWeights = TMap<NGeobase::TId, size_t>;

/*
* Class for finding BestGeoId in query using GeoAddr.
* Use mapping of tokens to their positions as workaround due to homonym dupications
* Examples: комарово -> 2 different tokens (Street and Quarter)
*/
class TGeoAddrMap {
public:
    struct TGeoAddrTokens {
        TGeoAddrTokens(bool sameAsQuery, TStringBuf normalized);

        NGeobase::TId BestId(bool useInherited, bool* isInherited) const;

        const bool SameAsQuery;
        const TString Normalized;
        TTokensWithWeights BestIds;
        TTokensWithWeights InheritedIds;
    };

public:
    void PrintTokens(IOutputStream* out) const;
    /** Returns first best geo id (or inherited best geo)
     * @param[out] id where geoid is placed
     * @param[out] isInherited is where flag if geoid id if from inherited is placed (otherwiset it always sets to false, event if id is NGeobase::UNKNOWN_REGION)
     * @return status if it has or hasn't best geo
     */
    TResultValue GetOneBestGeoId(NGeobase::TId& id, bool* isInherited = nullptr) const;
    TResultValue GetAllBestGeoIds(TVector<NGeobase::TId>& ids) const;

    const TGeoAddrTokens* GetOneBest() const;

    /** Constructs from raw text query.
     * It also finds the best normalized toponym <BestNormalizedToponym()>.
     * @param[in] ctx is Context from which it gets the slot
     * @param[in] requestText is a request text!!!
     * @param[in] useInherited if true return inherited best geo in case there is no normal best geo (use it with care because inherited is usually is not what you want)
     */
    static TGeoAddrMap FromRequest(TContext& ctx, TStringBuf requestText, bool useInherited = false);

    /** Constructs from 'where' slot. Get the text from 'where' slot and use <FromRequest()> to finish the job!
     * It is important that in case there is no 'where' slot it returns geoid ID based on current user location (from meta.location).
     * @see FromRequest()
     * @param[in] ctx is Context from which it gets the slot
     * @param[in] whereSlotName is a name of slot with place string
     * @param[in] useInherited if true return inherited best geo in case there is no normal best geo (use it with care because inherited is usually is not what you want)
     */
    static TGeoAddrMap FromSlot(TContext& ctx, TStringBuf slotName, bool useInherited = false);

    const TString& BestNormalizedToponym() const {
        return BestNormalized;
    }

    /** Check if geoaddr rule is emtpy.
     * It is also true in case it was created from 'where' slot and there was no slot value.
     */
    bool IsEmpty() const {
        return EmptyRule;
    }

private:
    using TGeoTokens = TMap<TString, TGeoAddrTokens>;

    const bool EmptyRule;
    const bool UseInherited;
    const bool IncorrectUserGeo;
    const TGeoTokens GeoTokens;
    const TString BestNormalized;

private:
    TGeoAddrMap(NGeobase::TId userRegion, TStringBuf bestNormalized);
    TGeoAddrMap(bool useInherited, TStringBuf bestNormalized, TGeoTokens&& geoTokens);
    static TGeoTokens GeoTokensFromUserRegion(NGeobase::TId userRegion);
};


/** Try to find normalized form of toponym in GeoAddr.
  * If nothing found, then return toponym in original form.
  */
TString NormalizeViaGeoAddr(TContext& ctx, const TStringBuf toponym);

}
