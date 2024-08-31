#include "config.h"
#include "backend_fs.h"
#include "backend_s3.h"

#include <alice/joker/library/log/log.h>

#include <library/cpp/config/config.h>

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/str.h>
#include <util/string/builder.h>

#if !defined(_win_)
extern char** environ;
#endif

namespace NAlice::NJoker {
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
    if (!parsedStream) {
        LOG(ERROR) << "Unable to create lua preprocessor stream!" << Endl;
        exit(EXIT_FAILURE);
    }
    NSc::TValue configJson = NSc::TValue::FromJson(parsedStream->ReadAll());
    if (jsonPatcher) {
        (*jsonPatcher)(&configJson);
    }
    return configJson;
}

THashSet<TString> ConstructSkipHeaders(const TConfig::TScheme& scheme) {
    THashSet<TString> headers;
    for (const auto& header : scheme.StubIdGenerator().SkipHeader()) {
        headers.emplace(to_lower(TString{*header}));
    }
    return headers;
}

THashSet<TString> ConstructSkipCGIs(const TConfig::TScheme& scheme) {
    THashSet<TString> cgis;
    for (const auto& cgi : scheme.StubIdGenerator().SkipCGI()) {
        cgis.emplace(to_lower(TString{*cgi}));
    }
    return cgis;
}

} // namespace

TConfig::TConfig(const TString& in, const TKeyValues& cmdVariables, TMaybe<TOnJsonPatch> jsonPatcher)
    : NSc::TValue{ParseConfig(in, cmdVariables, jsonPatcher)}
    , NJokerConfig::TConfig<TSchemeTraits>{this}
    , SessionsPath_{TFsPath{WorkDir()} / "sessions"}
    , SkipHeaders_{ConstructSkipHeaders(*this)}
    , SkipCGIs_{ConstructSkipCGIs(*this)}
    , SkipAllHeaders_{this->StubIdGenerator().SkipAllHeaders()}
    , SkipAllGCIs_{this->StubIdGenerator().SkipAllCGIs()}
    , SkipBody_{this->StubIdGenerator().SkipBody()}
{
    auto validate = [](TStringBuf path, TStringBuf error) {
        LOG(ERROR) << "Config: " << path << " : " << error << Endl;
    };
    if (!Validate("", false, validate)) {
        exit(EXIT_FAILURE);
    }

    SessionsPath_.MkDirs();
}

TConfig::TConfig(TConfig&& config)
    : NSc::TValue{std::move(config)}
    , NJokerConfig::TConfig<TSchemeTraits>{this}
    , SessionsPath_{std::move(config.SessionsPath_)}
{
}

const TFsPath& TConfig::SessionsPath() const {
    return SessionsPath_;
}

THolder<TBackend> TConfig::CreateBackend() const {
    static const THashMap<TStringBuf, std::function<THolder<TBackend>(const TConfig&)>> backends = {
        {
            TStringBuf("s3"),
            [](const TConfig& config) -> THolder<TBackend> {
                TConfig::TS3BackendConfigConst s3config(config.Backend().Params().GetRawValue());
                auto onError = [](TStringBuf path, TStringBuf msg) {
                    LOG(ERROR) << "S3 backend config: " << path << ": " << TString{msg}.Quote() << Endl;
                };
                Y_ENSURE(s3config.Validate("", false, onError), "Failed to validate jocker s3 config");
                return MakeHolder<TS3Backend>(s3config);
            }
        },
        {
            TStringBuf("sf"),
            [](const TConfig& config) -> THolder<TBackend> {
                TConfig::TFSBackendConfigConst fsConfig(config.Backend().Params().GetRawValue());
                auto onError = [] (TStringBuf path, TStringBuf msg) {
                    LOG(ERROR) << "FS backend config: " << path << ": " << TString{msg}.Quote() << Endl;
                };
                Y_ENSURE(fsConfig.Validate("", false, onError), "Failed to validate jocker fs config");
                return MakeHolder<TFSBackend>(fsConfig);
            }
        }
    };

    const auto* backendFactory = backends.FindPtr(Backend().Type());
    if (!backendFactory) {
        ythrow yexception() << "No backend with type " << TString{*Backend().Type()}.Quote() << " found.";
    }

    return (*backendFactory)(*this);
}

} // namespace NAlice::NJoker
