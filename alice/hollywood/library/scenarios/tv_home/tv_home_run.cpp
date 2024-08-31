#include "tv_home_run.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/scenarios/tv_home/nlg/register.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/proto/proto.h>

#include <alice/protos/data/video/tv_backend_request.pb.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf GET_CATEGORIES_FRAME = "alice.smarttv.get_categories";
constexpr TStringBuf GET_CAROUSELS_FRAME = "alice.smarttv.get_carousels";
constexpr TStringBuf GET_CAROUSEL_FRAME = "alice.smarttv.get_carousel";

template<typename T>
void FillParamsFromFrame(TCgiParameters& params, const TFrame& frame, TStringBuf name) {
    if (const auto slot = frame.FindSlot(name)) {
        auto value = slot->Value.template As<T>();
        if (!value.Empty()) {
            params.insert(std::make_pair(name, ToString(value.GetRef())));
        }
    }
}

template<typename T, typename TIterable>
void FillParamsFromFrame(TCgiParameters& params, const TFrame& frame, const TIterable& array) {
    for (const TStringBuf name: array) {
        FillParamsFromFrame<T>(params, frame, name);
    }
}

} // namespace

void TTvHomeRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    TString path;
    if (request.Input().FindSemanticFrame(GET_CATEGORIES_FRAME)) {
        path = TString("/api/v7/categories");
    } else if (const auto sframe = request.Input().FindSemanticFrame(GET_CAROUSELS_FRAME)) {
        TFrame frame = TFrame::FromProto(*sframe);
        TCgiParameters params;

        const std::array<TString, 3> stringArgs{"category_id", "cache_hash", "purchases_available_only"};
        FillParamsFromFrame<TString>(params, frame, stringArgs);

        const std::array<TString, 4> numArgs{"max_items_count", "limit", "offset", "restriction_age"};
        FillParamsFromFrame<ui32>(params, frame, numArgs);

        FillParamsFromFrame<bool>(params, frame, TStringBuf("kid_mode"));

        path = TString("/api/v7/carousels?") + params.Print();
    } else if (const auto sframe = request.Input().FindSemanticFrame(GET_CAROUSEL_FRAME)) {
        TFrame frame = TFrame::FromProto(*sframe);
        TCgiParameters params;

        const std::array<TString, 5> stringArgs{"carousel_id", "docs_cache_hash", "carousel_type", "filter", "tag", };
        FillParamsFromFrame<TString>(params, frame, stringArgs);

        const std::array<TString, 4> numArgs{"more_url_limit", "limit", "offset", "restriction_age"};
        FillParamsFromFrame<ui32>(params, frame, numArgs);

        const std::array<TString, 2> boolArgs{"kid_mode", "available_only"};
        FillParamsFromFrame<bool>(params, frame, boolArgs);

        path = TString("/api/v7/carousel?") + params.Print();
    }

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);

    if (path.empty()) {
        builder.SetIrrelevant();
        builder.SetError("unknown_frame", "");
    } else {
        NAlice::TTvBackendRequest args;
        args.SetPath(path);
        builder.SetContinueArguments(args);
        ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
    }
}

} // namespace NAlice::NHollywood
