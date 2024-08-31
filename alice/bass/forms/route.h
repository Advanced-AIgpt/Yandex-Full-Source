#pragma once

#include <alice/bass/forms/geo_resolver.h>
#include <alice/bass/forms/vins.h>
#include <alice/bass/forms/common/saved_address.h>
#include <library/cpp/enumbitset/enumbitset.h>

namespace NBASS {

struct ShowRouteDivCardData {
    NSc::TValue from;
    NSc::TValue to;
    NSc::TValue via;
    TStringBuf routeType;
    TStringBuf iconName;
    TStringBuf timeKeyPrefix = "time";
    TStringBuf showJams = "true";
};

class TRouteFormHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;
    bool FillToSlots(TContext& ctx);
    static void GetFormUpdate(const NSc::TValue& locationTo, NSc::TValue& formUpdate);

    // Switch request to return this form instead of original one.
    static TResultValue SetAsResponse(TContext& ctx);
    static TResultValue SetAsResponse(TContext& ctx, TStringBuf whatSlotName, TStringBuf whereSlotName);

    static void Register(THandlersMap* handlers);

private:
    enum ERouteOptions {
        BEGIN,
        ADDITIONAL_ROUTES = BEGIN,
        END
    };
    using TRouteOptions = TEnumBitSet<ERouteOptions, BEGIN, END>;
    TResultValue PushGalleryDivCardBlock(TContext& ctx, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to, const TContext::TSlot* slotRoute);
    TResultValue AddDivCardBlockByRouteType(TContext& ctx, NSc::TValue& cardData, ShowRouteDivCardData& data, const TRouteOptions opts = {}, const NSc::TValue& imageUrl = {});
    TResultValue AddDivCardBlockCar(TContext& ctx, NSc::TValue& cardData, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to, const NSc::TValue& imageUrl, const TRouteOptions opts = {});
    TResultValue AddDivCardBlockPublicTransport(TContext& ctx, NSc::TValue& cardData, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to);
    TResultValue AddDivCardBlockPedestrian(TContext& ctx, NSc::TValue& cardData, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to);
    TResultValue TryAddAllDivCardBlocks(TContext& ctx, NSc::TValue& cardData, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to);
    bool HasNamedLocation(const TContext& ctx) const;
    bool HasHowLongIntent(const TContext& ctx) const;
};

} // namespace NBASS
