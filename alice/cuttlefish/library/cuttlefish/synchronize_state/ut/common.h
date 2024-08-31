#pragma once

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/logging/dlog.h>

#include <apphost/lib/common/constants.h>
#include <apphost/lib/compression/compression.h>
#include <apphost/lib/compression/compression_codecs.h>
#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/registar.h>

#include <google/protobuf/text_format.h>
#include <google/protobuf/util/message_differencer.h>

#include <util/string/builder.h>


template <typename MessageType>
inline void AddProtobufInputItem(NAppHost::NService::TTestContext& ctx, TStringBuf source, TStringBuf type, const MessageType& message) {
    TStringBuilder data;
    NAppHost::NCompression::SetNoCompression(&data.Out);
    data << NAppHost::PROTOBUF_ITEM_PREFIX << message.SerializeAsString();
    ctx.AddInputItem(source, source, type, data);
}


template <typename MessageType>
inline void AddProtobufInputItem(NAppHost::NService::TTestContext& ctx, TStringBuf source, TStringBuf type, const TString& messageInTextFormat) {
    MessageType msg;
    Y_ENSURE(google::protobuf::TextFormat::ParseFromString(messageInTextFormat, &msg));
    AddProtobufInputItem(ctx, source, type, msg);
}


template <typename MessageType>
bool Contains(const NAppHost::TContextProtobufItemRefArray& items, const MessageType& msg, TMaybe<TStringBuf> type = Nothing())
{
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (type && it.GetType() != *type)
            continue;
        if (const auto maybeMsg = NAlice::NCuttlefish::TryParseProtobufItem<MessageType>(*it)) {
            DLOG("Compare with " << *maybeMsg);
            if (google::protobuf::util::MessageDifferencer::Equals(msg, *maybeMsg)) {
                return true;
            } else {
                DLOG("comparision failed for: " << maybeMsg->ShortUtf8DebugString());
            }
        }
    }
    return false;
}

template <typename MessageType>
inline bool Contains(const NAppHost::TContextProtobufItemRefArray& items, TStringBuf type, const MessageType& msg) {
    return Contains(items, msg, type);
}

template <typename MessageType>
inline bool Contains(const NAppHost::TContextProtobufItemRefArray& items, TStringBuf type, const TString& asText) {
    MessageType msg;
    Y_ENSURE(google::protobuf::TextFormat::ParseFromString(asText, &msg));
    return Contains(items, msg, type);
}
