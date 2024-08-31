#include "service.h"

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <util/string/builder.h>

using namespace NAlice::NTts;
using namespace NAlice::NCuttlefish;

// Debug helpers
template <typename T>
inline void AppHostDumpItems(const T& items, TStringBuf title, IOutputStream& out) {
    for (auto it = items.begin(), end = items.end(); it != end; ++it) {
        out << title << ": " << it.GetTag() << " " << it.GetType() << Endl;
    }
}

inline void AppHostDump(NAppHost::IServiceContext& ctx, IOutputStream& out) {
    AppHostDumpItems(ctx.GetRawInputItemRefs(), "Raw", out);
    AppHostDumpItems(ctx.GetItemRefs(NAppHost::EContextItemSelection::Anything), "Json", out);
    AppHostDumpItems(ctx.GetProtobufItemRefs(NAppHost::EContextItemSelection::Anything), "Proto", out);
}

void TService::TRequestProcessor::ProcessInput(TRequestHandlerPtr rh) {
    RequestHandler_ = std::move(rh);
    InitializeTts(CreateTtsCallbacks());

    ProcessInputImpl();
}

void TService::TRequestProcessor::ProcessInputImpl() {
    {
        if (!RequestHandler_->TryBeginProcessing()) {
            // Processing finished
            return;
        }

        ProcessBackendRequests();
        ProcessUnknownRequestItems();

        RequestHandler_->EndProcessing();
    }

    TIntrusivePtr<TRequestProcessor> self(this);
    RequestHandler_->Context().NextInput().Apply([processor = std::move(self)](auto hasData) mutable {
        if (!hasData.GetValue()) {
            processor->OnAppHostEmptyInput();
            processor->OnAppHostClose();
            return;
        }

        processor->ProcessInputImpl();
    });
}

void TService::TRequestProcessor::ProcessBackendRequests() {
    // Get all items is the only way to handle <prefix>_<something> item types for now
    const auto allInputRquestsItemRefs = RequestHandler_->Context().GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);
    for (auto it = allInputRquestsItemRefs.begin(); it != allInputRquestsItemRefs.end(); ++it) {
        if (const auto type = it.GetType();
            type == ITEM_TYPE_TTS_BACKEND_REQUEST ||
            type.StartsWith(ITEM_TYPE_TTS_BACKEND_REQUEST_PREFIX)
        ) {
            try {
                NProtobuf::TBackendRequest ttsBackendRequest;
                ParseProtobufItem(*it, ttsBackendRequest);
                OnBackendRequest(ttsBackendRequest, it.GetType());
            } catch (...) {
                OnError(CurrentExceptionMessage());
            }
        }

    }
}

void TService::TRequestProcessor::ProcessUnknownRequestItems() {
    const auto allInputRquestsItemRefs = RequestHandler_->Context().GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);
    for (auto it = allInputRquestsItemRefs.begin(); it != allInputRquestsItemRefs.end(); ++it) {
        // Skip known requests
        if (const auto type = it.GetType();
            type == ITEM_TYPE_TTS_BACKEND_REQUEST ||
            type.StartsWith(ITEM_TYPE_TTS_BACKEND_REQUEST_PREFIX)
        ) {
            continue;
        }

        OnUnknownItemType(TString(it.GetTag()), TString(it.GetType()));
    }
}

void TService::TRequestProcessor::OnBackendRequest(const NProtobuf::TBackendRequest& backendRequest, const TStringBuf& itemType) {
    DLOG("RequestProcessor.OnBackendRequest: " << backendRequest.ShortUtf8DebugString() << ", itemType=" << itemType);

    // Useful for logging
    Y_UNUSED(itemType);

    if (!Tts_) {
        return;
    }

    Tts_->ProcessBackendRequest(backendRequest);
}

void TService::TRequestProcessor::OnAppHostClose() {
    DLOG("RequestProcessor.OnAppHostClose");

    if (!Tts_) {
        return;
    }

    if (RequestHandler_->Context().IsCancelled()) {
        DLOG("RequestProcessor.OnAppHostClose (request is cancelled)");
        // Resquest in cancelled
        // no reason to wait generation
        Tts_->Cancel();
    }

    Tts_.Reset();
}
