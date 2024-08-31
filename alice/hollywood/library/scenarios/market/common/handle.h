#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/resources/geobase.h>
#include <alice/library/geo/user_location.h>

#include <library/cpp/geobase/lookup.hpp>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NMarket {

/*
    Class is DEPRECATED!
    Use TMarketRunContext/TMarketApplyContext instead
*/
template<class TRequestProto, class TRequestWrapper, class TResponseBuilder>
class TDeprecatedMarketHandleImplBase {
public:
    TDeprecatedMarketHandleImplBase(TScenarioHandleContext& ctx)
        : Ctx_(ctx)
    {}

    virtual void Do() = 0;

    bool HasExpFlag(const TStringBuf flag) const
    {
        return RequestWrapper().HasExpFlag(flag);
    }

protected:
    const TScenarioHandleContext& Ctx() const
    {
        return Ctx_;
    }

    TScenarioHandleContext& Ctx()
    {
        return Ctx_;
    }

    TRTLogger& Logger() const
    {
        return Ctx().Ctx.Logger();
    }

    TNlgWrapper& NlgWrapper()
    {
        if (NlgWrapper_.Empty()) {
            NlgWrapper_.ConstructInPlace(TNlgWrapper::Create(Ctx().Ctx.Nlg(), RequestWrapper(), Ctx().Rng, Ctx().UserLang));
        }
        return NlgWrapper_.GetRef();
    }

    const TRequestWrapper& RequestWrapper() const
    {
        if (RequestWrapper_.Defined()) {
            return RequestWrapper_.GetRef();
        }
        RequestProto = GetOnlyProtoOrThrow<TRequestProto>(Ctx().ServiceCtx, REQUEST_ITEM);
        return RequestWrapper_.ConstructInPlace(RequestProto.GetRef(), Ctx().ServiceCtx);
    }

    const NGeobase::TLookup& Geobase() const
    {
        return Ctx().Ctx.GlobalContext()
            .CommonResources().template Resource<TGeobaseResource>().GeobaseLookup();

    }

    TUserLocation CreateUserLocation(NGeobase::TId regionId) const
    {
        return {
            Geobase(),
            regionId,
            RequestWrapper().ClientInfo().Timezone
        };
    }

    void AddResponse(TResponseBuilder&& builder)
    {
        auto response = std::move(builder).BuildResponse();
        Ctx().ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
    }


private:
    TScenarioHandleContext& Ctx_;
    mutable TMaybe<TNlgWrapper> NlgWrapper_;
    mutable TMaybe<TRequestProto> RequestProto;
    mutable TMaybe<TRequestWrapper> RequestWrapper_;
};


class TDeprecatedMarketRunHandleImplBase
    : public TDeprecatedMarketHandleImplBase<
        TScenarioRunRequest,
        TScenarioRunRequestWrapper,
        TRunResponseBuilder
    > {
public:
    using TDeprecatedMarketHandleImplBase::TDeprecatedMarketHandleImplBase;

protected:
    NGeobase::TId GetUserRegionId() const;
    void SetIrrelevantResponse(TRunResponseBuilder& builder);
    void AddIrrelevantResponse();
};


class TDeprecatedMarketApplyHandleImplBase
    : public TDeprecatedMarketHandleImplBase<
        TScenarioApplyRequest,
        TScenarioApplyRequestWrapper,
        TApplyResponseBuilder
    > {
public:
    using TDeprecatedMarketHandleImplBase::TDeprecatedMarketHandleImplBase;
};

} // namespace NAlice::NHollywood::NMarket
