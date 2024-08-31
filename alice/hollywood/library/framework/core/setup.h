#pragma once

//
// HOLLYWOOD FRAMEWORK
// TSetup interface
//

#include "request.h"

#include <google/protobuf/message.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

//
// Forward declarations
//
namespace NAppHost {

class IServiceContext;

} // namespace NAppHost

namespace NAlice::NHollywood {

struct THttpProxyRequest;

} // namespace NAlice::NHollywood

namespace NAlice::NHollywoodFw {

//
// Forward declarations
//
class TProtoHwScene;
using THttpProxyRequest = NHollywood::THttpProxyRequest;

class TSetup {
public:
    TSetup(const TRequest& request)
        : Request_(request)
    {
    }
    TSetup(const TSetup& rhs) = default;
    inline const TRequest& GetRequest() const {
        return Request_;
    }

    // Attach any proto to outgoing requests
    // This is a simple version without controlling incoming items
    template <class TProto>
    void AttachRequest(TStringBuf outgoing, const TProto& proto) {
        TOutgoingRequests req;
        req.OutgoingName = outgoing;
        req.IncomingName = ""; // Not controlled
        TProto* copy = new TProto;
        copy->CopyFrom(proto);
        req.Msg.reset(copy);
        SetupList_.emplace_back(std::move(req));
    }

    //
    // Special functions to add predefined objects to request
    //

    // Attach http request to outgoing
    void Attach(const THttpProxyRequest& httpRequest, TStringBuf requestKey = "" /*PROXY_REQUEST_KEY_DEFAULT*/);


    inline size_t GetSetupCount() const {
        return SetupList_.size();
    }
    inline const google::protobuf::Message* FindMessage(TStringBuf outgoingName) const {
        const auto* ptr = FindIfPtr(SetupList_, [outgoingName](const TOutgoingRequests& it) {
            return it.OutgoingName == outgoingName;
        });
        return ptr ? ptr->Msg.get() : nullptr;
    }

    // Internal calls
    void MergeToCtx(NAppHost::IServiceContext& ctx, TProtoHwScene& sceneResults) const;
private:
    struct TOutgoingRequests {
        TString OutgoingName;
        TString IncomingName;
        std::shared_ptr<google::protobuf::Message> Msg;
    };
    TVector<TOutgoingRequests> SetupList_;
    const TRequest& Request_;
};

} // namespace NAlice::NHollywoodFw
