#pragma once

//
// HOLLYWOOD FRAMEWORK
// TSource interface
//

#include "request.h"
#include "setup.h"

#include <alice/library/proto/proto.h>

#include <apphost/api/service/cpp/service_context.h>

#include <google/protobuf/any.pb.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAppHost::NService {

class TProtobufItem;

} // namespace NAppHost::NService

namespace NAlice::NHollywoodFw {

namespace NPrivate {
    class TNodeCaller;
} // namespace NPrivate

class TSource : public TNonCopyable {
friend class NPrivate::TNodeCaller;
public:
    TSource(NAppHost::IServiceContext& ctx, const TRequest& request)
        : Ctx_(ctx)
        , Request_(request)
    {
    }

    // Get an answer from source (as protobuf)
    template <class TProto>
    bool GetSource(TStringBuf incoming, TProto& proto) const {
        // Find in precached items
        const auto& it = AllItems_.find(TString(incoming));
        if (it == AllItems_.end() || it->second == nullptr) {
            // Not found, try to extract directly from Ctx
            const auto& itemRefs = Ctx_.GetProtobufItemRefs(incoming);
            if (itemRefs.size() != 1) {
                return false;
            }
            proto = ParseProto<TProto>(itemRefs.front().Raw());
            return true;
        }
        proto = ParseProto<TProto>(it->second->Raw());
        return true;
    }

    //
    // Special functions to extract predefined answers from source
    //
    const NJson::TJsonValue GetHttpResponseJson(TStringBuf responseKey = "" /*PROXY_RESPONSE_KEY_DEFAULT*/, bool throwOnFailure = true) const;
    TMaybe<TProtoStringType> GetRawHttpContent(TStringBuf responseKey = "" /*PROXY_RESPONSE_KEY_DEFAULT*/, bool throwOnFailure = true) const;
    template <class TProto>
    TMaybe<TProto> GetHttpResponseProto(TStringBuf responseKey = "" /*PROXY_RESPONSE_KEY_DEFAULT*/,
                                bool throwOnFailure = true) const {
        TMaybe<TProtoStringType> content = GetRawHttpContent(responseKey, throwOnFailure);
        if (!content) {
            return Nothing();
        }

        TProto proto;
        if (!proto.ParseFromString(*content)) {
            const TStringBuf message = "Response failed to parse proto ";
            LOG_ERROR(Request_.Debug().Logger()) << message << proto.GetTypeName();
            if (throwOnFailure) {
                ythrow yexception() << message << proto.GetTypeName();
            }
        }
        return proto;
    }

    // Get a size of crecached items
    // Don't use this function if you doesn't specify incoming
    size_t Size() const {
        return AllItems_.size();
    }

private:
    bool GetSourceFromCtx(TStringBuf incoming, google::protobuf::Message& msg) const;
    // Internal calls
    void AddSourceResponse(const TString& outgoing, const TString& incoming, const NAppHost::NService::TProtobufItem* item) {
        Y_UNUSED(outgoing);
        AllItems_[incoming] = item;
    }

private:
    NAppHost::IServiceContext& Ctx_;
    const TRequest& Request_;
    TMap<TString, const NAppHost::NService::TProtobufItem*> AllItems_;
};

} // namespace NAlice::NHollywoodFw
