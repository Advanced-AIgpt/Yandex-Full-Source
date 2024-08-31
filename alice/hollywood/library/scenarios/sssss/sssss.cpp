#include "sssss.h"

#include <alice/hollywood/library/scenarios/sssss/proto/fast_data.pb.h>

#include <alice/hollywood/library/resources/geobase.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/registry/secret_registry.h>

#include <alice/library/logger/logger.h>

#include <util/stream/file.h>

namespace NAlice::NHollywood {

namespace {

const TString USELESS_TYPE = "stub";

} // namespace

class TSSSSResources final : public IResourceContainer {
public:
    void LoadFromPath(const TFsPath& dirPath) override {
        TFileInput in(dirPath / "sssss.txt");
        in.ReadLine(Data_);
    }

    const TString& Data() const {
        return Data_;
    }

private:
    TString Data_;
};

class TSSSSFastData : public IFastData {
public:
    TSSSSFastData(const TSSSSFastDataProto& proto)
        : SSSSString(proto.GetUselessFastData())
    {
    }

    const TString& GetString() const {
        return SSSSString;
    }

private:
    const TString SSSSString;
};

TString CreateResponseString(TContext& ctx) {
    auto& logger = ctx.Logger();

    const auto fastData = ctx.GlobalContext().FastData().GetFastData<TSSSSFastData>();
    const auto& resources = ctx.ScenarioResources<TSSSSResources>();
    const auto& geobase = ctx.GlobalContext().CommonResources().Resource<TGeobaseResource>().GeobaseLookup();
    LOG_INFO(logger) << "My fastData: " << fastData->GetString();

    TStringBuilder out;
    out << resources.Data() << fastData->GetString()
        << ". Glory to Mother " << geobase.GetRegionById(1).GetEnName() << "!";

    return out;
}

void TSimpleStupidRunHandle::Do(TScenarioHandleContext& ctx) const {
    TRunResponseBuilder response;
    response.SetError(USELESS_TYPE, CreateResponseString(ctx.Ctx));
    ctx.ServiceCtx.AddProtobufItem(*std::move(response).BuildResponse(), RESPONSE_ITEM);
}

void TSimpleStupidApplyHandle::Do(TScenarioHandleContext& ctx) const {
    TApplyResponseBuilder response;
    response.SetError(USELESS_TYPE, CreateResponseString(ctx.Ctx));
    ctx.ServiceCtx.AddProtobufItem(*std::move(response).BuildResponse(), RESPONSE_ITEM);
}

void TSimpleStupidCommitHandle::Do(TScenarioHandleContext& ctx) const {
    TCommitResponseBuilder response;
    response.SetError(USELESS_TYPE, CreateResponseString(ctx.Ctx));
    ctx.ServiceCtx.AddProtobufItem(*std::move(response).BuildResponse(), RESPONSE_ITEM);
}

REGISTER_SCENARIO("sssss",
                  AddHandle<TSimpleStupidRunHandle>()
                  .AddHandle<TSimpleStupidApplyHandle>()
                  .AddHandle<TSimpleStupidCommitHandle>()
                  .SetResources<TSSSSResources>()
                  .AddFastData<TSSSSFastDataProto, TSSSSFastData>("sssss/sssss.pb"));

REGISTER_SECRET("USELESS_SECRET", "DEFAULT_USELESS_VALUE");

} // namespace NAlice::NHollywood
