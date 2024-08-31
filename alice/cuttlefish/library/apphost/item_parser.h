#pragma once

#include <apphost/api/service/cpp/protobuf_item.h>

#include <util/generic/maybe.h>

namespace NAlice::NCuttlefish {

// Even if an exception occurs, the target targetProto will be partially filled
template<typename T>
void ParseProtobufItem(const NAppHost::NService::TProtobufItem& item, T& targetProto) {
    if (!item.Fill(&targetProto)) {
        static const auto* descriptor = T::descriptor();
        ythrow yexception() << "Fail parsing '" << descriptor->full_name() << "'";
    }
}

template<typename T>
T ParseProtobufItem(const NAppHost::NService::TProtobufItem& item) {
    T proto;
    ParseProtobufItem(item, proto);
    return proto;
}

template<typename T>
TMaybe<T> TryParseProtobufItem(const NAppHost::NService::TProtobufItem& item) {
    T proto;
    if (!item.Fill(&proto)) {
        return Nothing();
    }
    return proto;
}

} // namespace NAlice::NCuttlefish
