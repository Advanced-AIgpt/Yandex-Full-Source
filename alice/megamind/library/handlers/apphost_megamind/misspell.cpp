#include "misspell.h"

#include "components.h"
#include "node.h"
#include "on_utterance.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/request_builder.h>
#include <alice/megamind/library/apphost_request/util.h>

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/misspell/misspell.h>

#include <util/generic/strbuf.h>

namespace NAlice::NMegamind {
namespace {

void GetUtteranceFromEvent(TRequestComponentsView<TEventComponent> view, TMaybe<TString>& utterance) {
    const auto event = view.EventWrapper();
    if (!event) {
        return;
    }

    if (const auto& text = event->GetUtterance(); !text.Empty()) {
        utterance = text;
    }
}

} // namespace

TStatus AppHostMisspellSetup(IAppHostCtx& ahCtx, TRequestComponentsView<TClientComponent, TEventComponent> request, TMaybe<TString>& utterance) {
    TAppHostHttpProxyMegamindRequestBuilder requestBuilder;
    const auto event = request.EventWrapper();
    if (!event) {
        return TError{TError::EType::DataError} << "No event found";
    }
    const bool processMisspell = NeedMisspellRequest(event.Get(), request);
    auto result = CreateMisspellRequest(event->GetUtterance(),
                                        /* processMisspell= */ processMisspell,
                                        requestBuilder);

    if (!result.IsSuccess()) {
        return result.Status();
    }

    if (result.Value() == ESourcePrepareType::Succeeded) {
        requestBuilder.CreateAndPushRequest(ahCtx, AH_ITEM_MISSPELL_HTTP_REQUEST_NAME);
    } else {
        GetUtteranceFromEvent(request, utterance);
    }

    return Success();
}

TStatus AppHostMisspellPostSetup(IAppHostCtx& ahCtx, TMaybe<TString>& utterance) {
    NAppHostHttp::THttpResponse responseProto;
    if (auto err = GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_MISSPELL_HTTP_RESPONSE_NAME, responseProto)) {
        return std::move(*err);
    }

    if (responseProto.GetStatusCode() != HTTP_OK) {
        return TError{TError::EType::Critical} << "Misspell returned error: " << responseProto.ShortUtf8DebugString();

    }

    TMisspellProto misspell;
    if (auto err = ParseMisspellResponse(responseProto.GetContent()).MoveTo(misspell)) {
        return std::move(*err);
    }

    LOG_DEBUG(ahCtx.Log()) << "Misspell data: " << misspell.ShortUtf8DebugString();

    if (const auto& text = misspell.GetFixedText(); !text.Empty()) {
        ahCtx.ItemProxyAdapter().PutIntoContext(misspell, AH_ITEM_MISSPELL);
        utterance = text;
    }

    // No error means that there are no mistakes in original utterance have been found.
    return Success();
}

TStatus AppHostMisspellFromContext(IAppHostCtx& ahCtx, TMisspellProto& misspellProto) {
    return GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_MISSPELL, misspellProto);
}


} // namespace NAlice::NMegaamind
