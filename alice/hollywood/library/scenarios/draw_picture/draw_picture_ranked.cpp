#include "draw_picture_ranked.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/metrics/metrics.h>
#include <alice/library/logger/logger.h>

#include <util/string/cast.h>

#include <milab/lib/i2tclient/cpp/i2tclient.h>

using namespace NAlice::NScenarios;
using namespace NMilab::NI2t;

namespace NAlice::NHollywood {

    namespace {
        constexpr TStringBuf FEATURES_PATH = "cbirdaemon.dssm.Features.[0].Features";
    }

    void TDrawPictureRankedRunHandle::Do(TScenarioHandleContext& ctx) const {
        TDrawPictureImpl impl(ctx);

        NJson::TJsonValue responseItems;
        try {
            responseItems = RetireHttpResponseJson(ctx);
        } catch (const yexception& e) {
            LOG_ERR(ctx.Ctx.Logger()) << "I2T handle error: " << e;
            Fallback(impl);
            return;
        }

        const NJson::TJsonValue* jsonFeatures = responseItems.GetValueByPath(FEATURES_PATH);
        if (!jsonFeatures) {
            LOG_ERR(ctx.Ctx.Logger()) << "cannot find I2T features in JSON response by path: " << FEATURES_PATH;
            Fallback(impl);
            return;
        }
        const auto& array = jsonFeatures->GetArray();
        if (array.size() != NumI2tFeatures) {
            LOG_ERR(ctx.Ctx.Logger()) << "wrong number of features in JSON response: "
                                      << array.size() << " instead of " << NumI2tFeatures;
            Fallback(impl);
            return;
        }
        TI2tVector features;
        for (size_t i = 0; i < NumI2tFeatures; ++i) {
            features[i] = array[i].GetDouble();
        }

        impl.RenderDrawPicture(impl.GetRankedRandomImage(features));
    }

    void TDrawPictureRankedRunHandle::Fallback(TDrawPictureImpl &impl) const {
        NMonitoring::TLabels labels = ScenarioLabels(*this);
        labels.Add("name", "i2t_handle_failures");
        impl.Ctx.Ctx.GlobalContext().Sensors().IncRate(labels);
        impl.RenderDrawPicture(impl.GetRandomImage());
    }

} // namespace NAlice::NHollywood
