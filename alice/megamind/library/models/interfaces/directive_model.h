#pragma once

#include <alice/megamind/library/models/interfaces/model.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NAlice::NMegamind {

enum class EDirectiveType {
    ClientAction /* "client_action" */,
    ServerAction /* "server_action" */,
    UniproxyAction /* "uniproxy_action" */,
    ProtobufUniproxyAction /* "protobuf_uniproxy_action" */,
};

class IDirectiveModel : public IModel, public virtual TThrRefBase {
public:
    IDirectiveModel() = default;
    [[nodiscard]] virtual const TString& GetName() const = 0;
    [[nodiscard]] virtual EDirectiveType GetType() const = 0;
};

class TBaseDirectiveModel : public IDirectiveModel {
public:
    void SetEndpointId(const TString& endpointId);
    const TMaybe<TString>& GetEndpointId() const;

private:
    TMaybe<TString> EndpointId;
};

} // namespace NAlice::NMegamind
