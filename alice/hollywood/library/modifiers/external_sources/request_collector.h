#pragma once

#include <apphost/api/service/cpp/service_context.h>

namespace NAlice::NHollywood::NModifiers {

    struct IExternalSourceRequestCollector {
        virtual ~IExternalSourceRequestCollector() = default;
        virtual void AddRequest(const google::protobuf::Message& item, const TStringBuf type) = 0;
    };

    class TExternalSourceRequestCollector final : public IExternalSourceRequestCollector {
    public:
        explicit TExternalSourceRequestCollector(NAppHost::IServiceContext& apphostContext)
            : ApphostContext_(apphostContext)
        {
        }

        void AddRequest(const google::protobuf::Message& item, const TStringBuf type) final {
            ApphostContext_.AddProtobufItem(item, type);
        }
    private:
        NAppHost::IServiceContext& ApphostContext_;
    };

}
