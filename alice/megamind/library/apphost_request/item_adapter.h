#pragma once

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/util/status.h>

#include <alice/library/logger/logger.h>

#include <apphost/lib/proto_answers/http.pb.h>
#include <apphost/api/service/cpp/service_context.h>

#include <google/protobuf/message.h>

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

namespace NAlice::NMegamind {

using TAhHttpResponse = NAppHostHttp::THttpResponse;

class TItemProxyAdapter : private TMoveOnly {
public:
    using TProto = google::protobuf::Message;
    struct TExistsItemException : public yexception {
        using yexception::yexception;
    };

public:
    TItemProxyAdapter(NAppHost::IServiceContext& ctx, TRTLogger& logger, IGlobalCtx& globalCtx, bool useStreaming);

    void PutIntoContext(const TProto& item, const TStringBuf itemName);

    void PutJsonIntoContext(NJson::TJsonValue&& item, const TStringBuf itemName);

    void CheckAndPutIntoContext(const TProto& item, const TStringBuf itemName);

    TVector<TString> GetItemNamesFromCache();

    bool WaitNextInput();

    /** For each item in cache (without calling NextInput()).
     * Not thread safe!
     */
    template <typename TProtoType, typename TOnItemCallback>
    TStatus ForEachCached(const TStringBuf itemName, TOnItemCallback&& onItem) {
        UpdateCache();
        const auto* items = Items_.FindPtr(itemName);
        if (!items) {
            return TError{TError::EType::NotFound};
        }

        for (const auto& item : *items) {
            TProtoType proto;
            if (item.ToProto(proto)) {
                onItem(proto);
            } else {
                LOG_WARN(Logger_) << "Unable to parse input item '"<< itemName
                                  << "' from apphost cache: probably type-compatibility is broken";
            }
        }

        return Success();
    }

    template <typename TProtoType>
    TErrorOr<TProtoType> GetFromContextCached(const TStringBuf itemName, size_t index = 0) {
        UpdateCache();
        const auto* items = Items_.FindPtr(itemName);
        if (!items) {
            return TError{TError::EType::NotFound} << "Proto item '" << itemName << "' is not in context";
        }

        if (index >= items->size()) {
            return TError{TError::EType::Input} << "Proto item '"<< itemName << "' has total "
                                                << items->size() << " elements, " << index << " out of bounds";
        }

        TProtoType proto;
        if (!(*items)[index].ToProto(proto)) {
            return TError{TError::EType::Parse} << "unable to parse protobuf " << itemName << '[' << index << ']';
        }

        return std::move(proto);
    }

    template <typename TProtoType>
    TErrorOr<TProtoType> GetFromContext(const TStringBuf itemName, size_t index = 0) {
        UpdateCache();
        TString data;
        if (auto err = GetFromContextImpl(itemName, index).MoveTo(data)) {
            LOG_DEBUG(Logger_) << "Failed to retrieve " << itemName << " from app_host context: " << err;
            return std::move(*err);
        }

        TProtoType proto;
        Y_PROTOBUF_SUPPRESS_NODISCARD proto.ParseFromArray(data.data(), data.size());
        return std::move(proto);
    }

    TErrorOr<NJson::TJsonValue> GetJsonFromContext(const TStringBuf itemName, size_t index = 0);

    void AddBalancingHint(const TStringBuf source, const ui64 hint) {
        Ctx_.AddBalancingHint(source, hint);
    }

    const TMaybe<NJson::TJsonValue> GetAppHostParams() {
        const auto params = GetJsonFromContext(AH_ITEM_APPHOST_PARAMS);
        if (params.IsSuccess()) {
            return params.Value();
        }
        return Nothing();
    }

    void AddFlag(const TStringBuf flag) {
        Ctx_.AddFlag(flag);
    }

    bool CheckFlagInInputContext(const TStringBuf flag) const {
        return Ctx_.CheckFlagInInputContext(flag);
    }

    const NAppHost::TLocation& NodeLocation() const {
        return Location_;
    }

    void IntermediateFlush() {
        Ctx_.IntermediateFlush();
    }

    void AddLogLine(const TStringBuf text) {
        Ctx_.AddLogLine(text);
    }

private:
    TErrorOr<TString> GetFromContextImpl(const TStringBuf itemName, size_t index);
    void UpdateCache();

private:
    struct TProxyItem {
        TString Data;

        template <typename TProtoType>
        bool ToProto(TProtoType& proto) const {
            return proto.ParseFromArray(Data.data(), Data.size());
        }
    };
    NAppHost::IServiceContext& Ctx_;
    IGlobalCtx& GlobalCtx_;
    TRTLogger& Logger_;
    THashMap<TString, TVector<TProxyItem>> Items_;
    NAppHost::TLocation Location_;
    bool EndOfStreaming_ = false;
};

} // namespace NAlice::NMegamind
