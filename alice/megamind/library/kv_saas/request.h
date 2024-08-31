#pragma once

#include "response.h"

#include <alice/megamind/library/sources/request.h>
#include <alice/megamind/library/request_composite/client/client.h>
#include <alice/megamind/library/request_composite/event.h>
#include <alice/megamind/library/request_composite/view.h>
#include <alice/megamind/library/util/status.h>

#include <alice/library/network/request_builder.h>

#include <util/generic/string.h>

namespace NAlice::NKvSaaS {

TSourcePrepareStatus CreatePersonalIntentsRequest(NMegamind::TRequestComponentsView<NMegamind::TClientComponent> skr,
                                                  NNetwork::IRequestBuilder& request);
TSourcePrepareStatus CreateQueryTokensStatsRequest(const TString& utterance,
                                                   NMegamind::TRequestComponentsView<NMegamind::TClientComponent> skr,
                                                   NNetwork::IRequestBuilder& request);

template <typename T>
TErrorOr<T> ParseResponse(const TString& content) {
    T response;
    try {
        if (auto err = response.Parse(content)) {
            return std::move(*err);
        }
    } catch (...) { // Any parsing error.
        return TError{TError::EType::Critical} << "KvSaaS response parsing error: " << CurrentExceptionMessage();
    }

    return response;
}

} // namespace NAlice::NKvSaaS
