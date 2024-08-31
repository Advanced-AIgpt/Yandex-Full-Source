#pragma once

#include "item_adapter.h"

#include <alice/megamind/library/apphost_request/protos/error.pb.h>

#include <alice/megamind/library/response_meta/error.h>
#include <alice/megamind/library/util/status.h>

#include <apphost/api/service/cpp/service_context.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

#include <functional>

namespace NAlice::NMegamind {

/** Push error to apphost context as standard item with type 'mm_error'.
 */
void PushErrorItem(NAppHost::IServiceContext& ctx, const TErrorMetaBuilder& errorMetaBuilder,
                   const NAppHost::EContextItemKind kind = NAppHost::EContextItemKind::Output);
/** This function also updates Location in errorMetaBuild from ServiceContext.
 */
void PushErrorItem(NAppHost::IServiceContext& ctx, TErrorMetaBuilder&& errorMetaBuilder,
                   const NAppHost::EContextItemKind kind = NAppHost::EContextItemKind::Output);

template <typename T>
TStatus GetFirstProtoItem(TItemProxyAdapter& itemProxyAdapter, const TStringBuf name, T& proto) {
    auto item = itemProxyAdapter.GetFromContext<T>(name);
    if (!item.IsSuccess()) {
        return *item.Error();
    }
    item.MoveTo(proto);

    return Success();
}

template <typename T>
TErrorOr<T> GetFirstProtoItem(TItemProxyAdapter& itemProxyAdapter, const TStringBuf name) {
    T proto;
    if (auto err = GetFirstProtoItem(itemProxyAdapter, name, proto)) {
        return std::move(*err);
    }

    return std::move(proto);
}

using TJsonRef = std::reference_wrapper<const NJson::TJsonValue>;
TStatus GetFirstJsonItem(TItemProxyAdapter& itemProxyAdapter, const TStringBuf name, TJsonRef& json);

TErrorOr<TJsonRef> GetFirstJsonItem(TItemProxyAdapter& itemProxyAdapter, const TStringBuf name);

NMegamindAppHost::TErrorProto ErrorToProto(const TError& error);

TError ErrorFromProto(const NMegamindAppHost::TErrorProto& proto);

void LogSkrInfo(TItemProxyAdapter& itemProxyAdapter, const TStringBuf uuid, const TStringBuf messageId, const int hypothesisNumber);

} // namespace NAlice::NMegamind
