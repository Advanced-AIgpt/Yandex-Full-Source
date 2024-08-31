#pragma once

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/util/service_context.h>

#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood {

class THwServiceContext {
public:

    THwServiceContext(IGlobalContext& globalContext, NAppHost::IServiceContext& apphostContext, TRTLogger& logger);

    IGlobalContext& GlobalContext() {
        return GlobalContext_;
    }

    NAppHost::IServiceContext& ApphostContext() {
        return ApphostContext_;
    }

    const NAppHost::IServiceContext& ApphostContext() const {
        return ApphostContext_;
    }

    TRTLogger& Logger() {
        return Logger_;
    }

    template <typename TProto>
    [[nodiscard]] TProto GetProtoOrThrow(const TStringBuf type) const {
        return GetOnlyProtoOrThrow<TProto>(ApphostContext(), type);
    }

    template <typename TProto>
    [[nodiscard]] TMaybe<TProto> GetMaybeProto(const TStringBuf type) const {
        return GetMaybeOnlyProto<TProto>(ApphostContext(), type);
    }

    template <typename TProto>
    [[nodiscard]] TVector<TProto> GetProtos(const TStringBuf type) const {
        auto items = ApphostContext().GetProtobufItemRefs(type);
        TVector<TProto> protos(Reserve(items.size()));
        for (const auto& item : items) {
            protos.push_back(ParseProto<TProto>(item.Raw()));
        }
        return protos;
    }

    TVector<TString> GetItemNamesFromContext() const {
        TVector<TString> itemNames;
        const auto& items = ApphostContext().GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);
        for (auto srcIt = items.begin(), srcEnd = items.end(); srcIt != srcEnd; ++srcIt) {
            itemNames.push_back(TString{srcIt.GetType()});
        }
        return itemNames;
    }

    void AddProtobufItemToApphostContext(const google::protobuf::Message& item,
                                         const TStringBuf& type);

    void AddBalancingHint(TStringBuf source, ui64 hint);

    NMetrics::ISensors& Sensors();

private:
    IGlobalContext& GlobalContext_;
    TRTLogger& Logger_;
    NAppHost::IServiceContext& ApphostContext_;
};

} // namespace NAlice::NHollywood
