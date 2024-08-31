#pragma once

#include <library/cpp/threading/future/future.h>

namespace google::protobuf {
    class Message;
}

namespace NRTLogAgent {

    class TFrameInfo;
    class TPQConfig;

    class IMessageSender {
    public:
        virtual ~IMessageSender() = default;

        virtual NThreading::TFuture<void> Send(const google::protobuf::Message& message) = 0;
    };

    THolder<IMessageSender> MakeLogbrokerMessageSender(const TPQConfig& queueConfig);
}
