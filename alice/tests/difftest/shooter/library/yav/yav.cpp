#include "yav.h"

#include <alice/joker/library/log/log.h>

#include <util/string/builder.h>

#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/neh.h>
#include <library/cpp/scheme/scheme.h>

namespace NAlice::NShooter {

namespace {

THashMap<TString, TString> ConstructYav(TString response) {
    TStringStream ss{response};
    NJson::TJsonValue jsonValue = NJson::ReadJsonTree(&ss, /* throwOnError = */ true);
    Y_ASSERT(jsonValue["status"].GetString() == "ok");

    THashMap<TString, TString> result;
    for (const auto& kv : jsonValue["version"]["value"].GetArray()) {
        result[kv["key"].GetString()] = kv["value"].GetString();
    }
    return result;
}

THashMap<TString, TString> ConstructYav(const TString& secretId, const TString& oauthToken, const IYavRequester& requester) {
    if (!secretId.Empty()) {
        TString response = requester.Request(secretId, oauthToken);
        return ConstructYav(std::move(response));
    }
    return {};
}

} // namespace


IYav::IYav(THashMap<TString, TString>&& map)
    : THashMap{std::move(map)}
{
}

TYav::TYav(const TString& secretId, const TString& oauthToken, const IYavRequester& requester)
    : IYav{ConstructYav(secretId, oauthToken, requester)}
{
}

} // namespace NAlice::NShooter
