#pragma once

#include <alice/bass/libs/config/config.sc.h>
#include <alice/bass/libs/fetcher/fwd.h>
#include <alice/bass/clients.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/domscheme_traits.h>

#include <util/generic/fwd.h>
#include <util/generic/maybe.h>

/** Config parser class.
 * It is possible to use a small lua blocks within config.
 * The lua preprocessor is run before json parsing. The lua environment has
 * all the os environemnt variables with prefix 'ENV_'.
 * (i.e. if there is os env variable LC_ALL it is possible to get its value by writing "${ ENV_LC_ALL }").
 * If requested variable is undefined than constructor throws yexception.
 * @code
 * { "some_key": "${ ENV_SUPER_DUPER_ENV_VARIABLE }", "other_key": "ok" }
 * @endcode
 */
class TConfig : private NSc::TValue,
                public NBASSConfig::TConfig<TSchemeTraits> {
public:
    using TKeyValues = TVector<TString>;
    using TOnJsonPatch = std::function<void(NSc::TValue*)>;
    using TYdbScheme = NBASSConfig::TYdbConfigConst<TSchemeTraits>;

public:
    TConfig(const TString& in, ui16 httpPort, const TKeyValues& cmdVariables,
            TMaybe<TOnJsonPatch> jsonPatcher = Nothing(), NBASS::EBASSClient client = NBASS::EBASSClient::Alice,
            const TString& currentDc = Default<TString>());
    TConfig(TConfig&& cfg);

    TConfig DeepClone() const;

    const TString* GetCurrentDC() const;

    ui16 GetHttpPort() const {
        return HttpPort;
    }

    ui16 GetMonitoringPort() const {
        return static_cast<ui16>(HttpPort + 1);
    }

    NHttpFetcher::TProxySettingsPtr GetProxyOverride() const;

    std::unique_ptr<TConfig> PatchedConfig(const NSc::TValue& patch) const;

    const NSc::TValue& AsJson() const {
        return static_cast<const NSc::TValue&>(*this);
    }

    NBASS::EBASSClient BASSClient() const {
        return Client;
    }

    TConfig::TTvm2Source Tvm2() {
        switch (BASSClient()) {
            case NBASS::EBASSClient::Alice:
            case NBASS::EBASSClient::DevNoMusic:
                return Vins().Tvm2();
            case NBASS::EBASSClient::Crmbot:
                return Crmbot().Tvm2();
        }
    }

    TConfig::TTvm2SourceConst Tvm2() const {
        switch (BASSClient()) {
            case NBASS::EBASSClient::Alice:
            case NBASS::EBASSClient::DevNoMusic:
                return Vins().Tvm2();
            case NBASS::EBASSClient::Crmbot:
                return Crmbot().Tvm2();
        }
    }

private:
    ui16 HttpPort;
    NHttpFetcher::TProxySettingsPtr ProxyOverride;
    NBASS::EBASSClient Client;
    TMaybe<TString> CurrentDc;

    // Do not allow copying of config
    TConfig(const TConfig& cfg);
    TConfig& operator=(const TConfig& cfg);

    TConfig(NSc::TValue json, TMaybe<TString> currentDc);
};
