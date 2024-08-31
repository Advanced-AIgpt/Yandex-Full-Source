#include "geodb.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/geo_resolver.h>
#include <alice/bass/forms/geoaddr.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/globalctx/globalctx.h>

#include <alice/library/geo/geodb.h>

#include <util/generic/strbuf.h>
#include <util/string/ascii.h>
#include <util/string/builder.h>

namespace NBASS {
TRequestedGeo::TRequestedGeo(IGlobalContext& globalCtx)
    : Error(TError(TError::EType::NOGEOFOUND, TStringBuf("no geo found")))
    , Same(true)
    , SpecialLocation(TSpecialLocation::EType::ERROR)
    , GlobalCtx(globalCtx)
{
}

TRequestedGeo::TRequestedGeo(const TRequestedGeo& geo)
    : Region(geo.Region)
    , Error(geo.Error)
    , Same(geo.Same)
    , SpecialLocation(geo.SpecialLocation)
    , GlobalCtx(geo.GlobalCtx)
{
}

TRequestedGeo& TRequestedGeo::operator=(const TRequestedGeo& geo) {
    Region = geo.Region;
    Error = geo.Error;
    Same = geo.Same;
    SpecialLocation = geo.SpecialLocation;
    GlobalCtx = geo.GlobalCtx;
    return *this;
}

TRequestedGeo::TRequestedGeo(TRequestedGeo&& geo)
    : Region(std::move(geo.Region))
    , Error(std::move(geo.Error))
    , Same(geo.Same)
    , SpecialLocation(geo.SpecialLocation)
    , GlobalCtx(geo.GlobalCtx)
{
}

TRequestedGeo::TRequestedGeo(TContext& ctx, TStringBuf slotName,
                                bool failIfNoSlot, const TStringBuf geoType)
    : TRequestedGeo(ctx, ctx.GetSlot(slotName), failIfNoSlot, geoType)
{
}

TRequestedGeo::TRequestedGeo(TContext& ctx, const TContext::TSlot* whereSlot,
                                bool failIfNoSlot, const TStringBuf geoType)
    : Same(true)
    , SpecialLocation(TSpecialLocation::EType::ERROR)
    , GlobalCtx(ctx.GlobalCtx())
{
    NGeobase::TId geoId = NGeobase::UNKNOWN_REGION;

    if (!IsSlotEmpty(whereSlot)) {
        const auto& type = whereSlot->Type;
        if (type == TStringBuf("string") || type == TStringBuf("geo_adjective")) {
            Error = ParseAsQuery(ctx, *whereSlot, geoId, Same, geoType);
        } else if (type == TStringBuf("special_location")) {
            Error = ParseAsSpecialLocation(ctx, *whereSlot, geoId, SpecialLocation);
        } else if (type == TStringBuf("geo_id")) {
            Error = ParseAsId(*whereSlot, geoId);
        } else if (type == TStringBuf("sys.geo") || type == TStringBuf("geo")) {
            Error = ParseAsSysGeo(*whereSlot, geoId);
        } else if (type == TStringBuf("GeoAddr.Address")) {
            Error = ParseAsGeoAddr(*whereSlot, geoId);
        }
        else {
            Error = TError(TError::EType::NOTSUPPORTED,
                            TStringBuilder() << "Unsupported 'where' slot type: "
                                            << whereSlot->Type);
        }

        if (Error) {
            return;
        }
    } else {
        if (failIfNoSlot) {
            Error = TError(TError::EType::NOGEOFOUND, "Slot 'where' is empty");
            return;
        }

        geoId = ctx.UserRegion();
        if (!NAlice::IsValidId(geoId)) {
            Error =
                TError(TError::EType::NOUSERGEO, TStringBuf("No user geo found"));
            return;
        }
    }

    InitGeoPtr(geoId);
}

TRequestedGeo::TRequestedGeo(IGlobalContext& globalCtx, NGeobase::TId geoId,
                                bool same)
    : Error(TResultValue())
    , Same(same)
    , SpecialLocation(TSpecialLocation::EType::ERROR)
    , GlobalCtx(globalCtx)
{
    InitGeoPtr(geoId);
}

TRequestedGeo::TRequestedGeo(IGlobalContext& globalCtx, NGeobase::TId geoid)
    : TRequestedGeo(globalCtx, geoid, true /* same */)
{
}

void TRequestedGeo::ConvertTo(NGeobase::TId geoId) {
    if (geoId == GetId()) {
        return;
    }

    Same = false;
    Error.Clear();
    InitGeoPtr(geoId);
}

bool TRequestedGeo::ConvertTo(NGeobase::ERegionType type) {
    if (!IsValidId()) {
        return false;
    }

    // decide that if current node has the same type
    // do not do nothing and say it's ok!
    if (GetGeoType() == type) {
        return true;
    }

    NGeobase::TId id = GetParentIdByType(type);
    if (!NAlice::IsValidId(id)) {
        return false;
    }

    ConvertTo(id);
    return true;
}

TResultValue TRequestedGeo::CreateResolvedMeta(TContext& ctx,
                                                TStringBuf slotName,
                                                bool isSame) const {
    if (Error) {
        return Error;
    }

    NSc::TValue slotValue;
    AddAllCaseForms(ctx, &slotValue, true);
    slotValue["geoid"].SetIntNumber(GetId());

    TContext::TSlot* slot = ctx.GetOrCreateSlot(slotName, TStringBuf("geo"));
    Y_ASSERT(slot);
    slot->Value = std::move(slotValue);

    if (!Same || !isSame) {
        ctx.AddAttention(TStringBuf("geo_changed"), NSc::Null());
    }

    return TResultValue();
}

TResultValue TRequestedGeo::ParseAsQuery(TContext& ctx,
                                            const TContext::TSlot& slot,
                                            NGeobase::TId& geoId, bool& same,
                                            const TStringBuf geoType) const {
    const TGeoAddrMap geoAddrMap(TGeoAddrMap::FromRequest(
        ctx, slot.Value.GetString(), false /* useInherited */));
    if (geoAddrMap.IsEmpty()) {
        return TError(TError::EType::NOGEOFOUND,
                        TStringBuilder() << "No geo found: " << slot.Value);
    }

    const auto error = geoAddrMap.GetOneBestGeoId(geoId);
    if (!error) {
        Y_ASSERT(NAlice::IsValidId(geoId));
        same = true;
        return TResultValue();
    }

    const TString& normalizedToponym = geoAddrMap.BestNormalizedToponym();
    if (!normalizedToponym) {
        return error;
    }

    TGeoObjectResolver geoResolver(
        ctx, normalizedToponym,
        InitGeoPositionFromLocation(ctx.Meta().Location()), geoType);
    NSc::TValue location;
    if (geoResolver.WaitAndParseResponse(&location)) {
        return error;
    }

    NGeobase::TId gi = location["geoid"].GetIntNumber(NGeobase::UNKNOWN_REGION);
    if (!NAlice::IsValidId(gi)) {
        return TError(TError::EType::NOGEOFOUND,
                        TStringBuilder() << "No geo found: " << slot.Value);
    }

    geoId = gi;
    same = location["level"].GetString() == TStringBuf("city");
    return TResultValue();
}

TResultValue TRequestedGeo::ParseAsSpecialLocation(
    TContext& ctx, const TContext::TSlot& slot, NGeobase::TId& geoId,
    TSpecialLocation& specialLocation) const {
    TSpecialLocation sl = TSpecialLocation::GetNamedLocation(&slot);
    if (sl == TSpecialLocation::EType::ERROR) {
        return TError(TError::EType::NOGEOFOUND,
                        TStringBuilder() << "Can't get named location from slot: "
                                        << slot.Value);
    }

    TResultValue error;
    NGeobase::TId gi = sl.GetGeo(ctx, &error);
    if (!NAlice::IsValidId(gi)) {
        if (!error) {
            error = TError(TError::EType::NOUSERGEO,
                            TStringBuilder() << "Unsupported special location: "
                                            << SpecialLocation);
        }
        return error;
    }

    geoId = gi;
    specialLocation = sl;
    return TResultValue();
}

TResultValue TRequestedGeo::ParseAsId(const TContext::TSlot& slot, NGeobase::TId& geoId) const {
    NGeobase::TId gi = slot.Value.GetIntNumber(NGeobase::UNKNOWN_REGION);
    if (!NAlice::IsValidId(gi)) {
        return TError(TError::EType::NOUSERGEO, TStringBuilder() << "Slot with type geo_id is invalid: "
                                                                 << slot.Value);
    }

    geoId = gi;
    return TResultValue();
}

TResultValue TRequestedGeo::ParseAsSysGeo(const TContext::TSlot& slot, NGeobase::TId& geoId) const {
    const TString slotValue = slot.Value.IsString() ? TString{slot.Value.GetString()} : slot.Value.ToJson();
    const TStringBuf slotStr = slotValue;

    size_t begin = FindIndexIf(slotStr, [](const char c) { return IsAsciiDigit(c); });
    if (begin == TStringBuf::npos) {
        return TError(TError::EType::NOUSERGEO,
                      TStringBuilder() << "Slot with type sys.geo is invalid: "
                                       << slot.Value);
    }
    size_t end = FindIndexIf(slotStr.substr(begin), [](const char c) { return !IsAsciiDigit(c); });

    geoId = FromString(slotStr.substr(begin, end));
    return TResultValue();
}

TResultValue TRequestedGeo::ParseAsGeoAddr(const TContext::TSlot& slot, NGeobase::TId& geoId) const {
    const TStringBuf slotValue = slot.Value.IsString() ? TString{slot.Value.GetString()} : slot.Value.ToJson();
    NJson::TJsonValue json;
    if (!NJson::ReadJsonTree(slotValue, &json, /* throwOnError = */ false)) {
        return TError(TError::EType::NOUSERGEO,
                      TStringBuilder() << "Slot with type GeoAddr.Address is invalid: " << slotValue);
    }

    for (const auto& key : {"BestGeoId", "BestInheritedId"}) {
        if (!json.Has(key)) {
            continue;
        }
        const NJson::TJsonValue& value = json[key];
        if (value.IsInteger()) {
            geoId = value.GetInteger();
            if (NAlice::IsValidId(geoId)) {
                return TResultValue();
            }
        }
    }
    return TError(TError::EType::NOUSERGEO,
                  TStringBuilder() << "Slot with type GeoAddr.Address is invalid: " << slotValue);
}

void TRequestedGeo::InitGeoPtr(NGeobase::TId geoId) {
    // reset Region
    Region.Clear();

    if (NAlice::IsValidId(geoId)) {
        Region = GlobalCtx.GeobaseLookup().GetRegionById(geoId);
    }

    if (!Region || !IsValidId()) {
        Error = TError(TError::EType::NOGEOFOUND, TStringBuf("no geo found"));
    }
}

bool TRequestedGeo::HasError() const {
    return !!Error;
}

bool TRequestedGeo::IsValidId() const {
    return NAlice::IsValidId(GetId());
}

TResultValue TRequestedGeo::GetError() const {
    return Error;
}

NGeobase::TId TRequestedGeo::GetId() const {
    if (!Region) {
        return NGeobase::UNKNOWN_REGION;
    }
    return GetRegion().GetId();
}

bool TRequestedGeo::IsSame() const {
    return Same;
}

NGeobase::ERegionType TRequestedGeo::GetGeoType() const {
    if (!Region) {
        return NGeobase::ERegionType::OTHER;
    }
    return GetRegion().GetEType();
}

NGeobase::TRegion TRequestedGeo::GetRegion() const {
    return Region.GetRef();
}

NGeobase::TId TRequestedGeo::GetParentIdByType(NGeobase::ERegionType type) const {
    if (!IsValidId()) {
        return NGeobase::UNKNOWN_REGION;
    }
    return GlobalCtx.GeobaseLookup().GetParentIdWithType(GetId(), static_cast<int>(type));
}

NGeobase::TId TRequestedGeo::GetParentCityId() const {
    return NAlice::ReduceGeoIdToCity(GlobalCtx.GeobaseLookup(), GetId());
}

void TRequestedGeo::GetNames(TStringBuf lang, TString* name, TString* namePrepcase) const {
    NAlice::GeoIdToNames(GlobalCtx.GeobaseLookup(), GetId(), lang, name, namePrepcase);
}

TStringBuf TRequestedGeo::ToTld() const {
    return NAlice::GeoIdToTld(GlobalCtx.GeobaseLookup(), GetId());
}

TString TRequestedGeo::GetTimeZone() const {
    if (!Region) {
        return TString();
    }
    return GetRegion().GetTimezoneName().data();
}

void TRequestedGeo::AddAllCaseForms(const TContext& ctx, NSc::TValue* geoJson, bool wantObsolete) const {
    NAlice::AddAllCaseForms(GlobalCtx.GeobaseLookup(), GetId(), ctx.MetaLocale().Lang, geoJson, wantObsolete);
}

void TRequestedGeo::AddAllCaseFormsWithFallbackLanguage(const TContext& ctx, NSc::TValue* geoJson, bool wantObsolete) const {
    NAlice::AddAllCaseFormsWithFallbackLanguage(GlobalCtx.GeobaseLookup(), GetId(), ctx.MetaLocale().Lang, geoJson, wantObsolete);
}

} // namespace NBASS
