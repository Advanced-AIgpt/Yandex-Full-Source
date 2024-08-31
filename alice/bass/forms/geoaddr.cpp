#include "geoaddr.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/neh/neh.h>

#include <util/charset/wide.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS {
namespace {

bool BestIdComparator(const std::pair<NGeobase::TId, size_t>& l, const std::pair<NGeobase::TId, size_t>& r) {
    return l.second < r.second;
}

NGeobase::TId GetTheBestIds(const TTokensWithWeights& ids) {
    auto it = std::max_element(
        ids.cbegin(),
        ids.cend(),
        BestIdComparator
    );
    return it == ids.cend() ? NGeobase::UNKNOWN_REGION : it->first;
}

} // anon namespace

NGeobase::TId TGeoAddrMap::TGeoAddrTokens::BestId(bool useInherited, bool* isInherited) const {
    if (isInherited) {
        *isInherited = false;
    }

    if (BestIds.size()) {
        NGeobase::TId geoId = GetTheBestIds(BestIds);
        if (NAlice::IsValidId(geoId)) {
            return geoId;
        }
    }

    if (!useInherited) {
        return NGeobase::UNKNOWN_REGION;
    }

    NGeobase::TId geoId = GetTheBestIds(InheritedIds);
    if (NAlice::IsValidId(geoId) && isInherited) {
        *isInherited = true;
    }
    return geoId;
}

TGeoAddrMap::TGeoTokens TGeoAddrMap::GeoTokensFromUserRegion(NGeobase::TId userRegion) {
    TGeoTokens geoTokens;
    if (NAlice::IsValidId(userRegion)) {
        // it is a fake geotoken
        ++geoTokens.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(TStringBuf()),
            std::forward_as_tuple(false, TStringBuf())
        ).first->second.BestIds[userRegion];
    }
    return geoTokens;
}


TGeoAddrMap::TGeoAddrMap(bool useInherited, TStringBuf bestNormalized, TGeoTokens&& geoTokens)
    : EmptyRule(false)
    , UseInherited(useInherited)
    , IncorrectUserGeo(false)
    , GeoTokens(std::move(geoTokens))
    , BestNormalized(std::move(bestNormalized))
{
}

TGeoAddrMap::TGeoAddrMap(NGeobase::TId userRegion, TStringBuf bestNormalized)
    : EmptyRule(true)
    , UseInherited(false)
    , IncorrectUserGeo(!NAlice::IsValidId(userRegion))
    , GeoTokens(GeoTokensFromUserRegion(userRegion))
    , BestNormalized(bestNormalized)
{
}

TGeoAddrMap TGeoAddrMap::FromRequest(TContext& ctx, TStringBuf request, bool useInherited) {
    TCgiParameters cgi;
    TStringBuf uil = ctx.MetaLocale().Lang;
    TStringBuf tld = ctx.UserTld();
    if (!uil.empty()) {
        cgi.InsertEscaped(TStringBuf("uil"), ctx.MetaLocale().Lang);
    }
    if (!tld.empty()) {
        cgi.InsertEscaped(TStringBuf("tld"), tld);
    }
    const NSc::TValue rwd = ctx.ReqWizard(request, ctx.UserRegion(), cgi);

    const NSc::TValue& gar = rwd["rules"]["GeoAddr"];
    if (gar.IsNull()) {
        return TGeoAddrMap(ctx.UserRegion(), request);
    }

    TVector<NSc::TValue> found;
    const NSc::TValue& ua = gar["UnfilteredAnswer"];
    if (ua.IsNull()) {
        LOG(WARNING) << "No unfiltered answer found in geoaddr rule" << Endl;
        LOG(DEBUG) << "REQWIZARD ANSWER (no unfiltered): " << rwd.ToJson() << Endl;
        return TGeoAddrMap(ctx.UserRegion(), request);
    }

    if (ua.IsArray()) {
        for (const NSc::TValue& ans : ua.GetArray()) {
            found.emplace_back(NSc::TValue::FromJson(ans.GetString()));
        }
    }
    else {
        found.emplace_back(NSc::TValue::FromJson(ua.GetString()));
    }

    NGeobase::TId userCityId = NAlice::ReduceGeoIdToCity(ctx.GlobalCtx().GeobaseLookup(), ctx.UserRegion());
    const i64 queryNumTokens = rwd["markup"]["Tokens"].ArraySize();

    TStringBuf bestNormalized(request);
    bool bestUserCity = false;
    NSc::TValue bestGeoAddr;

    TGeoTokens geoTokens;
    for (const NSc::TValue& garItem : found) {
        const NSc::TValue& body = garItem["Body"];
        // this magic number copied from proxywizard's parser geoaddr which is made by yulika@
        // she said that it is the same as was in perl report
        if (body["Weight"].GetNumber() < 0.1) {
            continue;
        }

        NGeobase::TId bgid = body["BestGeo"].GetIntNumber(NGeobase::UNKNOWN_REGION);
        NGeobase::TId igid = useInherited ? body["BestInheritedGeo"].GetIntNumber(NGeobase::UNKNOWN_REGION) : NGeobase::UNKNOWN_REGION;
        const i64 pos = garItem["Pos"].ForceIntNumber();
        const i64 len = garItem["Length"].ForceIntNumber();

        if (NAlice::IsValidId(bgid) || NAlice::IsValidId(igid)) {
            const TString key{TStringBuilder() << pos << len}; // FIXME key
            TGeoAddrTokens* geo = geoTokens.FindPtr(key);
            if (!geo) {
                geo = &geoTokens.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(key),
                    std::forward_as_tuple(len == queryNumTokens, garItem["NormalizedText"].GetString())
                ).first->second;
            }

            ++geo->BestIds[bgid];
            if (NAlice::IsValidId(igid)) {
                ++geo->InheritedIds[igid];
            }
        }

        // Это только для того что бы найти лучше нормализованый запрос
        // Find best geoaddr parsing - first element with user's city_id, or just first element (if user's city_id not found)
        // Берём только те варианты разбора правила GeoAddr, которые покрывают все леммы (токены) пользовательского запроса
        // Например, для "кафе на парке культуры" правило GeoAddr вернёт "парк культуры". Такие варианты пропускаем
        if (!pos && len == queryNumTokens) {
            if (bgid == userCityId || igid == userCityId) {
                bestUserCity = true;
                bestGeoAddr = garItem;
            }
            else if (bestGeoAddr.IsNull()) {
                bestGeoAddr = garItem;
            }
        }
    }

    // Build normalized name from best geoaddr parsing
    if (!bestGeoAddr.IsNull()) {
        bestNormalized = bestGeoAddr["NormalizedText"].GetString();
    }

    return TGeoAddrMap(useInherited, bestNormalized, std::move(geoTokens));
}

TGeoAddrMap::TGeoAddrTokens::TGeoAddrTokens(bool sameAsQuery, TStringBuf normalized)
    : SameAsQuery(sameAsQuery)
    , Normalized(normalized)
{
}

TGeoAddrMap TGeoAddrMap::FromSlot(TContext& ctx, TStringBuf whereSlotName, bool useInherited) {
    TContext::TSlot* whereSlot = ctx.GetSlot(whereSlotName);
    if (!IsSlotEmpty(whereSlot)) {
        return FromRequest(ctx, whereSlot->Value.GetString(), useInherited);
    }

    return TGeoAddrMap(ctx.UserRegion(), TStringBuf(""));
}

void TGeoAddrMap::PrintTokens(IOutputStream* out) const {
    for (const auto& gt : GeoTokens) {
        *out << '"' << gt.first << "\" : " << gt.second.BestIds.size();
        if (UseInherited) {
            *out << "," << gt.second.InheritedIds.size() << Endl;
        }
        for (const auto ids : gt.second.BestIds) {
            *out << "\t" << ids.first << " is " << ids.second << Endl;
        }
        if (UseInherited) {
            for (const auto ids : gt.second.InheritedIds) {
                *out << "\t\t" << ids.first << " is " << ids.second << Endl;
            }
        }
    }
}

TResultValue TGeoAddrMap::GetOneBestGeoId(NGeobase::TId& id, bool* isInherited) const {
    if (GeoTokens.size() < 1 && IncorrectUserGeo) {
        return TError(
            TError::EType::NOUSERGEO,
            "incorrect user geo"
        );
    }
    if (GeoTokens.size() != 1) {
        return TError(
            TError::EType::NOGEOFOUND,
            TStringBuilder() << "geotokens must be 1, but we have " << GeoTokens.size()
        );
    }

    id = GeoTokens.cbegin()->second.BestId(UseInherited, isInherited);
    return TResultValue();
}

const TGeoAddrMap::TGeoAddrTokens* TGeoAddrMap::GetOneBest() const {
    if (IncorrectUserGeo || GeoTokens.size() != 1) {
        return nullptr;
    }

    const TGeoAddrTokens* token = &GeoTokens.cbegin()->second;
    Y_ASSERT(token);
    // just check if it has best geo
    if (!NAlice::IsValidId(token->BestId(false, nullptr /* isInherited */))) {
        return nullptr;
    }

    return token;
}

TResultValue TGeoAddrMap::GetAllBestGeoIds(TVector<NGeobase::TId>& ids) const {
    for (auto& gt : GeoTokens) {
        NGeobase::TId id = gt.second.BestId(UseInherited, nullptr);
        if (!NAlice::IsValidId(id)) {
            return TError(
                TError::EType::NOGEOFOUND,
                "some geotokens has not BestGeoID"
            );
        }
        ids.push_back(id);
    }
    return TResultValue();
}

} // namespace NBASS
