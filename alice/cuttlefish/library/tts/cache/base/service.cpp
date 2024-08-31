#include "service.h"

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

using namespace NAlice::NTtsCache;
using namespace NAlice::NCuttlefish;

void TService::TRequestProcessor::ProcessInput(TRequestHandlerPtr rh) {
    RequestHandler_ = std::move(rh);
    InitializeTtsCache(CreateTtsCacheCallbacks());

    ProcessInputImpl();
}

void TService::TRequestProcessor::ProcessInputImpl() {
    {
        if (!RequestHandler_->TryBeginProcessing()) {
            // processing finished
            return;
        }

        ProcessCacheSetRequests();
        ProcessCacheGetRequests();
        ProcessCacheWarmUpRequests();
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

void TService::TRequestProcessor::ProcessCacheSetRequests() {
    const auto itemRefs = RequestHandler_->Context().GetProtobufItemRefs(ITEM_TYPE_TTS_CACHE_SET_REQUEST, NAppHost::EContextItemSelection::Input);
    for (const auto& itemRef : itemRefs) {
        try {
            NProtobuf::TCacheSetRequest cacheSetRequest;
            ParseProtobufItem(itemRef, cacheSetRequest);
            OnCacheSetRequest(cacheSetRequest);
        } catch (...) {
            OnError(CurrentExceptionMessage());
        }
    }
}

void TService::TRequestProcessor::ProcessCacheGetRequests() {
    const auto itemRefs = RequestHandler_->Context().GetProtobufItemRefs(ITEM_TYPE_TTS_CACHE_GET_REQUEST, NAppHost::EContextItemSelection::Input);
    for (const auto& itemRef : itemRefs) {
        try {
            NProtobuf::TCacheGetRequest cacheGetRequest;
            ParseProtobufItem(itemRef, cacheGetRequest);
            OnCacheGetRequest(cacheGetRequest);
        } catch (...) {
            OnError(CurrentExceptionMessage());
        }
    }
}

void TService::TRequestProcessor::ProcessCacheWarmUpRequests() {
    const auto itemRefs = RequestHandler_->Context().GetProtobufItemRefs(ITEM_TYPE_TTS_CACHE_WARM_UP_REQUEST, NAppHost::EContextItemSelection::Input);
    for (const auto& itemRef : itemRefs) {
        try {
            NProtobuf::TCacheWarmUpRequest cacheWarmUpRequest;
            ParseProtobufItem(itemRef, cacheWarmUpRequest);
            OnCacheWarmUpRequest(cacheWarmUpRequest);
        } catch (...) {
            OnError(CurrentExceptionMessage());
        }
    }
}

void TService::TRequestProcessor::ProcessUnknownRequestItems() {
    const auto allInputRquestsItemRefs = RequestHandler_->Context().GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);
    for (auto it = allInputRquestsItemRefs.begin(); it != allInputRquestsItemRefs.end(); ++it) {
        // Skip known requests
        if (const auto type = it.GetType();
            type == ITEM_TYPE_TTS_CACHE_SET_REQUEST
            || type == ITEM_TYPE_TTS_CACHE_GET_REQUEST
            || type == ITEM_TYPE_TTS_CACHE_WARM_UP_REQUEST
        ) {
            continue;
        }

        OnUnknownItemType(TString(it.GetTag()), TString(it.GetType()));
    }
}

void TService::TRequestProcessor::OnCacheSetRequest(const NProtobuf::TCacheSetRequest& cacheSetRequest) {
    DLOG("RequestProcessor.OnCacheSetRequest: " << cacheSetRequest.GetKey());

    if (!TtsCache_) {
        return;
    }

    TtsCache_->ProcessCacheSetRequest(cacheSetRequest);
}

void TService::TRequestProcessor::OnCacheGetRequest(const NProtobuf::TCacheGetRequest& cacheGetRequest) {
    DLOG("RequestProcessor.OnCacheGetRequest: " << cacheGetRequest.ShortUtf8DebugString());

    if (!TtsCache_) {
        return;
    }

    TtsCache_->ProcessCacheGetRequest(cacheGetRequest);
}

void TService::TRequestProcessor::OnCacheWarmUpRequest(const NProtobuf::TCacheWarmUpRequest& cacheWarmUpRequest) {
    DLOG("RequestProcessor.OnCacheWarmUpRequest: " << cacheWarmUpRequest.ShortUtf8DebugString());

    if (!TtsCache_) {
        return;
    }

    TtsCache_->ProcessCacheWarmUpRequest(cacheWarmUpRequest);
}

void TService::TRequestProcessor::OnAppHostClose() {
    DLOG("RequestProcessor.OnAppHostClose");

    if (!TtsCache_) {
        return;
    }

    if (RequestHandler_->Context().IsCancelled()) {
        DLOG("RequestProcessor.OnAppHostClose (request is cancelled)");
        TtsCache_->Cancel();
    }

    TtsCache_.Reset();
}
