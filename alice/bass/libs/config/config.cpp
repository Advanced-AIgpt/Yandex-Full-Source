#include "config.h"

#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/config/config.h>

#include <util/generic/yexception.h>
#include <util/stream/str.h>
#include <util/string/builder.h>

#if !defined(_win_)
extern char** environ;
#endif

namespace {

NSc::TValue ParseConfig(const TString& in, const TConfig::TKeyValues& cmdVariables, TMaybe<TConfig::TOnJsonPatch> jsonPatcher) {
    NConfig::TGlobals globals;

#if !defined(_win_)
    for (char** env = environ; *env; ++env) {
        TStringBuf name, value;
        if (TStringBuf(*env).TrySplit('=', name, value)) {
            globals[TStringBuilder() << TStringBuf("ENV_") << name] = value;
        }
    }
#endif

    for (const auto& kvStr : cmdVariables) {
        const size_t pos = kvStr.find('=');
        if (pos == TStringBuf::npos) {
            ythrow yexception() << "ERROR: invalid key=value for " << kvStr;
        }

        globals[kvStr.substr(0, pos)] = kvStr.substr(pos + 1);
    }

    TStringInput inStream(in);
    THolder<IInputStream> parsedStream{NConfig::CreatePreprocessor(globals, inStream)};
    if (Y_UNLIKELY(!parsedStream)) {
        LOG(ERR) << "Unable to create lua preprocessor stream!" << Endl;
        exit(1);
    }
    NSc::TValue configJson = NSc::TValue::FromJsonThrow(parsedStream->ReadAll());
    if (jsonPatcher) {
        (*jsonPatcher)(&configJson);
    }
    return configJson;
}

} // namespace

TConfig::TConfig(const TString& in, ui16 httpPort, const TKeyValues& cmdVariables, TMaybe<TOnJsonPatch> jsonPatcher,
                 NBASS::EBASSClient client, const TString& currentDc)
    : TConfig(ParseConfig(in, cmdVariables, jsonPatcher), Nothing())
{
    HttpPort = httpPort > 0 ? httpPort : NBASSConfig::TConfig<TSchemeTraits>::HttpPort();
    Client = client;
    if (!currentDc.Empty()) {
        CurrentDc.ConstructInPlace(currentDc);
    }
}

TConfig::TConfig(NSc::TValue json, TMaybe<TString> currentDc)
    : NSc::TValue(std::move(json))
    , NBASSConfig::TConfig<TSchemeTraits>(this)
    , CurrentDc(std::move(currentDc))
{
    TStringBuilder errmsg;
    auto validate = [&errmsg](TStringBuf path, TStringBuf error) {
        if (!errmsg.empty()) {
            errmsg << ';';
        }
        errmsg << path << " : " << error;
    };
    if (!Validate({}, false, validate)) {
        LOG(ERR) << "Config: " << errmsg << Endl;
        ythrow yexception() << errmsg;
    }

    const auto fetcherProxy = FetcherProxy();
    if (!fetcherProxy.HostPort()->empty()) {
        if (auto proxyOverride = NHttpFetcher::TProxySettings::CreateFromHeader(*fetcherProxy.HostPort())) {
            for (const auto h : fetcherProxy.Headers()) {
                proxyOverride->AddHeader(TString{*h.Name()}, TString{*h.Value()});
            }
            ProxyOverride = proxyOverride;
        } else {
            LOG(ERR) << "Unable to create proxy settings for '" << *fetcherProxy.HostPort() << Endl;
        }
    }
    Client = NBASS::EBASSClient::Alice;
}

TConfig::TConfig(const TConfig& cfg)
    : NSc::TValue(cfg.GetRawValue()->Clone())
    , NBASSConfig::TConfig<TSchemeTraits>(this)
    , ProxyOverride(cfg.ProxyOverride)
    , Client(cfg.Client)
    , CurrentDc(cfg.CurrentDc)
{
}

TConfig::TConfig(TConfig&& cfg)
    : NSc::TValue(std::move(static_cast<NSc::TValue&>(cfg)))
    , NBASSConfig::TConfig<TSchemeTraits>(this)
    , ProxyOverride(std::move(cfg.ProxyOverride))
    , Client(std::move(cfg.Client))
    , CurrentDc(std::move(cfg.CurrentDc))
{
}

TConfig TConfig::DeepClone() const {
   return *this;
}

NHttpFetcher::TProxySettingsPtr TConfig::GetProxyOverride() const {
    return ProxyOverride;
}

const TString* TConfig::GetCurrentDC() const {
    return CurrentDc.Get();
}

std::unique_ptr<TConfig> TConfig::PatchedConfig(const NSc::TValue& patch) const {
    if (patch.IsNull()) {
        return {};
    }

    NSc::TValue json = Clone();
    json.MergeUpdate(patch);

    return std::unique_ptr<TConfig>(new TConfig(std::move(json), CurrentDc));
}
