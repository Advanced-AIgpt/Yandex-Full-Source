#include "megamind.h"

#include <alice/megamind/protos/speechkit/request.pb.h>

void NAlice::NCuttlefish::Censore(NAlice::TSpeechKitRequestProto& skRequest) {
    // remove oauth token& cookies
    if (skRequest.HasRequest()) {
        auto& request = *skRequest.MutableRequest();
        if (request.HasAdditionalOptions()) {
            auto& additionalOptions = *request.MutableAdditionalOptions();
            if (additionalOptions.HasOAuthToken()) {
                additionalOptions.SetOAuthToken("<censored>");
            }
            if (additionalOptions.HasICookie()) {
                additionalOptions.SetICookie("<censored>");
            }
            if (additionalOptions.HasBassOptions()) {
                auto& bassOptions = *additionalOptions.MutableBassOptions();
                for (int i = 0; i < bassOptions.GetCookies().size(); ++i) {
                    (*bassOptions.MutableCookies())[i] = "<censored>";
                }
            }
        }
        if (request.HasMegamindCookies()) {
            request.SetMegamindCookies("<censored>");
        }
    }
}
