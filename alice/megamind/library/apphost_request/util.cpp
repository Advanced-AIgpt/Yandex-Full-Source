#include "util.h"
#include "item_names.h"

#include <alice/megamind/library/response_meta/error.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <util/generic/hash.h>

namespace NAlice::NMegamind {

// Rest functions --------------------------------------------------------------
void PushErrorItem(NAppHost::IServiceContext& ctx, const TErrorMetaBuilder& errorMetaBuilder, const NAppHost::EContextItemKind kind) {
    ctx.AddProtobufItem(errorMetaBuilder.AsProtoItem(), AH_ITEM_ERROR, kind);
}

void PushErrorItem(NAppHost::IServiceContext& ctx, TErrorMetaBuilder&& errorMetaBuilder, const NAppHost::EContextItemKind kind) {
    errorMetaBuilder.SetNetLocation(ctx.GetLocation().Path);
    PushErrorItem(ctx, errorMetaBuilder, kind);
}

TStatus GetFirstJsonItem(TItemProxyAdapter& itemProxyAdapter, const TStringBuf name, TJsonRef& json) {
    auto item = itemProxyAdapter.GetJsonFromContext(name);
    if (!item.IsSuccess()) {
        return TError{TError::EType::NotFound} << "no json item '" << name << "' found";
    }

    json = item.Value();

    return Success();
}

TErrorOr<TJsonRef> GetFirstJsonItem(TItemProxyAdapter& itemProxyAdapter, const TStringBuf name) {
    NJson::TJsonValue val;
    TJsonRef json(std::cref(val));
    if (auto err = GetFirstJsonItem(itemProxyAdapter, name, json)) {
        return std::move(*err);
    }
    return std::move(json);
}

NMegamindAppHost::TErrorProto ErrorToProto(const TError& error) {
    NMegamindAppHost::TErrorProto proto;
    proto.SetType(ToString(error.Type));
    proto.SetMessage(error.ErrorMsg);
    if (error.HttpCode.Defined()) {
        proto.SetHttpCode(static_cast<int>(*error.HttpCode));
    }
    return proto;
}

TError ErrorFromProto(const NMegamindAppHost::TErrorProto& proto) {
    try {
        TError error;
        error.Type = FromString<TError::EType>(proto.GetType());
        if (proto.HasHttpCode()) {
            error.HttpCode = static_cast<HttpCodes>(proto.GetHttpCode());
        }
        error.ErrorMsg << proto.GetMessage();
        return error;
    } catch(...) {
        return TError{TError::EType::Parse} << "Failed to parse proto: " << proto.DebugString()
                                            << ", exception: " << CurrentExceptionMessage();
    }
}

void LogSkrInfo(TItemProxyAdapter& itemProxyAdapter, const TStringBuf uuid, const TStringBuf messageId, const int hypothesisNumber) {
    if (uuid.empty() && messageId.empty()) {
        return;
    }

    NJson::TJsonValue json;
    json["message_id"] = messageId;
    json["uuid"] = uuid;
    json["hypothesis_number"] = hypothesisNumber;
    itemProxyAdapter.AddLogLine(TString::Join("skr_info", NJson::WriteJson(json, /* formatOutput= */false, /* sortkeys= */true)));
}

} // namespace NAlice::NMegamind
