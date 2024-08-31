#include "maps_download_offline.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/hollywood/library/resources/geobase.h>

#include <alice/library/geo/geodb.h>
#include <alice/library/proto/proto.h>
#include <alice/library/url_builder/url_builder.h>

#include <util/generic/variant.h>
#include <util/string/cast.h>

#include <library/cpp/string_utils/quote/quote.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

const TString FRAME = "alice.maps.download_offline";
const TString FRAME_UNCERTAINLY = "alice.maps.download_offline_uncertainly";

const TString SLOT_TYPE_ADDRESS = "GeoAddr.Address";
const TString SLOT_TYPE_SYS_GEO = "sys.geo";

} // namespace

void TMapsDownloadOfflineRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TNlgData nlgData{ctx.Ctx.Logger(), request};

    bool uncertainly = false;
    auto maybeFrame = request.Input().TryCreateRequestFrame(FRAME);
    if (maybeFrame.Empty()) {
        maybeFrame = request.Input().TryCreateRequestFrame(FRAME_UNCERTAINLY);
        uncertainly = true;
    }

    TFrame* frame = maybeFrame.Get();
    TString regionName;
    if (const auto regionPtr = frame->FindSlot("region")) {
        const TStringBuf slotType = regionPtr->Type;
        const TStringBuf slotValue = regionPtr->Value.AsString();

        if (slotType == SLOT_TYPE_ADDRESS || slotType == SLOT_TYPE_SYS_GEO) {
            NGeobase::TId id;
            TGeoUtilStatus errorStatus;

            if (slotType == SLOT_TYPE_ADDRESS) {
                errorStatus = ParseGeoAddrAddress(slotValue, id);
            } else {
                errorStatus = ParseSysGeo(slotValue, id);
            }

            if (errorStatus.Empty()) {
                const auto& geobase = ctx.Ctx.GlobalContext().CommonResources().Resource<TGeobaseResource>().GeobaseLookup();
                regionName = geobase.GetRegionById(id).GetName();
            } else {
                LOG_ERROR(ctx.Ctx.Logger()) << "Failed to construct region name: " << errorStatus->Message();
            }
        }
    }

    TCgiParameters cgi;
    cgi.InsertUnescaped("start_download", "true");
    if (regionName.empty()) {
        cgi.InsertUnescaped("search_by", "current_camera_position");
    } else {
        cgi.InsertUnescaped("search_by", "name");
        cgi.InsertUnescaped("name", regionName);
    }

    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder(frame);

    auto& analyticsInfo = bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfo.SetProductScenarioName("maps_download_offline");

    const auto& interfaces = request.Proto().GetBaseRequest().GetInterfaces();
    if (interfaces.GetSupportsMapsDownloadOffline()) {
        TDirective directive;
        TOpenUriDirective& openUriDirective = *directive.MutableOpenUriDirective();
        openUriDirective.SetUri("yandexmaps://maps.yandex.ru/offline-maps?" + PrintCgi(cgi));
        bodyBuilder.AddDirective(std::move(directive));
        bodyBuilder.AddRenderedTextWithButtonsAndVoice("maps_download_offline", "download_will_start", {}, nlgData);
    } else if (interfaces.GetCanOpenLinkIntent()) {
        TDirective directive;
        TOpenUriDirective& openUriDirective = *directive.MutableOpenUriDirective();
        openUriDirective.SetUri(GenerateMapsUri(request.ClientInfo(), GetUserLocation(request), "/offline-maps", cgi, "", true, false, "https://mobile.yandex.ru/apps/maps"));
        bodyBuilder.AddDirective(std::move(directive));
        bodyBuilder.AddRenderedTextWithButtonsAndVoice("maps_download_offline", "redirect", {}, nlgData);
        if (uncertainly) {
            builder.SetIrrelevant();
        }
    } else {
        bodyBuilder.AddRenderedTextWithButtonsAndVoice("maps_download_offline", "stub", {}, nlgData);
        if (uncertainly) {
            builder.SetIrrelevant();
        }
    }

    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

REGISTER_SCENARIO("maps_download_offline",
                  AddHandle<TMapsDownloadOfflineRunHandle>().SetNlgRegistration(
                      NAlice::NHollywood::NLibrary::NScenarios::NMapsDownloadOffline::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
