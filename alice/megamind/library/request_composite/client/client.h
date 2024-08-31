#pragma once

#include <alice/megamind/protos/common/experiments.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>

#include <alice/library/client/client_features.h>
#include <alice/library/client/client_info.h>
#include <alice/library/experiments/utils.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>

namespace NAlice::NMegamind {

class TClientComponent {
public:
    using TExpFlags = THashMap<TString, TMaybe<TString>>;

    class TView {
    public:
        explicit TView(const TClientComponent& client)
            : Client_{client}
        {
        }

        const TString* ClientIp() const {
            return Client_.ClientIp();
        }

        const TString* AuthToken() const {
            return Client_.AuthToken();
        }

        const TClientFeatures& ClientFeatures() const {
            return Client_.ClientFeatures();
        }

        const TClientInfo& ClientInfo() const {
            return Client_.ClientFeatures();
        }

        bool HasExpFlag(TStringBuf name) const;

        TMaybe<TString> ExpFlag(TStringBuf name) const;

        const TExpFlags& ExpFlags() const;

        const TDeviceState& DeviceState() const {
            return Client_.DeviceState();
        }

    private:
        const TClientComponent& Client_;
    };

public:
    virtual ~TClientComponent() = default;

    virtual const TString* ClientIp() const = 0;
    virtual const TString* AuthToken() const = 0;
    virtual const TClientFeatures& ClientFeatures() const = 0;
    virtual const TExpFlags& ExpFlags() const = 0;
    virtual const TDeviceState& DeviceState() const = 0;

protected:
    static TExpFlags CreateExpFlags(const TExperimentsProto& proto);
    static TClientFeatures CreateClientFeatures(const TSpeechKitRequestProto& proto, const TExpFlags& expFlags);
};

class TExpFlagsConverter final : public IExpFlagVisitor {
public:
    static TClientComponent::TExpFlags Build(const TExperimentsProto& proto) {
        TExpFlagsConverter visitor;
        visitor.Visit(proto);
        return visitor.ExpFlags;
    }

protected:
    void operator()(const TString& key, bool value) override {
        ExpFlags.emplace(key, ToString(value));
    }
    void operator()(const TString& key, double value) override {
        ExpFlags.emplace(key, ToString(value));
    }
    void operator()(const TString& key, const TString& value) override {
        ExpFlags.emplace(key, value);
    }
    void operator()(const TString& key) override {
        ExpFlags.emplace(key, Nothing());
    }

private:
    TExpFlagsConverter() = default;

private:
    TClientComponent::TExpFlags ExpFlags;
};

} // namespace NAlice::NMegamind
