#include "transform_face_run.h"
#include "transform_face_impl.h"

#include <alice/hollywood/library/global_context/global_context.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf TEXT_FOR_STATION = (
    "Я бы с удовольствием, но не могу трансформировать внешность на этом устройстве. "
    "Попросите меня об этом в приложении Яндекса, и я с радостью!"
);

}

void TTransformFaceRunHandle::Do(TScenarioHandleContext& ctx) const {
    TTransformFaceRunImpl impl(ctx);

    const auto& clientInfo = impl.Request.ClientInfo();
    if (clientInfo.IsNavigator() || clientInfo.IsYaAuto() || clientInfo.IsElariWatch() ||
        clientInfo.IsSmartSpeaker() || clientInfo.IsTvDevice() || clientInfo.IsYaBrowserDesktop()) {
        impl.Reject(TEXT_FOR_STATION, false);
        return;
    }

    auto transformRequest = impl.GetTransformRequest();

    if (transformRequest.HasType()) {
        impl.Continue(transformRequest);
        return;
    }

    if (!impl.IsFaceTransformRequest()) {
        impl.Reject("", true);
        return;
    }

    impl.RequestPhoto();
}

} // namespace NAlice::NHollywood
