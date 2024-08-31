#include "draw_picture.h"
#include "draw_picture_impl.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <util/string/cast.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

    namespace {

        constexpr TStringBuf TEXT_FOR_NAVIGATOR =
            "Простите, я не могу показать вам картину прямо сейчас. Попросите меня об этом в приложении Яндекса."
        ;
        constexpr TStringBuf TEXT_FOR_STATION =
            "Я бы с удовольствием, но не могу показать вам картину на этом устройстве. Попросите меня об этом в приложении Яндекса, и я с радостью!"
        ;
        constexpr TStringBuf TEXT_FOR_ROUTE =
            "Маршрут я могу построить, если попросите, а картину могу нарисовать."
        ;

    } // namespace

    void TDrawPictureRunHandle::Do(TScenarioHandleContext& ctx) const {
        TDrawPictureImpl impl(ctx);
        const auto& clientInfo = impl.Request.ClientInfo();

        if (clientInfo.IsNavigator() || clientInfo.IsYaAuto() || clientInfo.IsElariWatch()) {
            impl.RejectDrawPicture(TEXT_FOR_NAVIGATOR, true);
            return;
        }

        if (clientInfo.IsSmartSpeaker() || clientInfo.IsTvDevice()) {
            impl.RejectDrawPicture(TEXT_FOR_STATION, false);
            return;
        }

        TString slotValue;
        if (const auto fromPtr = impl.Frame.FindSlot(TStringBuf("request"))) {
            slotValue = fromPtr->Value.AsString();
        }

        if (slotValue.empty()) {
            impl.RenderDrawPicture(impl.GetRandomImage());
            return;
        }

        if (slotValue.Contains("маршрут")) {
            impl.RejectDrawPicture(TEXT_FOR_ROUTE, true);
            return;
        }

        // if slot contains one of special values we respond with predefined images (substitutes)
        const auto maybeResponse = impl.LookupSubstitutes(slotValue);
        if (maybeResponse.Defined()) {
            impl.RenderDrawPicture(*maybeResponse);
            return;
        }

        // in this case rendering happens in TDrawPictureRankedRunHandle
        TCgiParameters params = {
            std::make_pair("cbird", "49"),
            std::make_pair("type", "json"),
            std::make_pair("text", slotValue),
        };
        auto path = TString("?") + params.Print();
        auto httpRequest = PrepareHttpRequest(path, ctx.RequestMeta, ctx.Ctx.Logger());
        AddHttpRequestItems(ctx, httpRequest);
    }

} // namespace NAlice::NHollywood
