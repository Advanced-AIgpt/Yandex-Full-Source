#include "util.h"

#include <library/cpp/uri/uri.h>
#include <library/cpp/json/json_writer.h>

#include <util/folder/filelist.h>
#include <util/folder/dirut.h>
#include <util/string/builder.h>
#include <util/system/fs.h>

namespace NAlice::NShooter {

void CopyDir(const TString& source, const TString& destination) {
    NFs::MakeDirectory(destination);
    TFileEntitiesList fl(TFileEntitiesList::EM_FILES_DIRS);
    fl.Fill(source, TStringBuf(), TStringBuf(), /* depth = */ 100);
    while (const char * filename = fl.Next()) {
        if (IsDir(source + "/" + TString(filename))) {
            MakeDirIfNotExist((destination + "/" + TString(filename)).c_str());
        } else {
            NFs::Copy(source + "/" + filename, destination + "/" + filename);
        }
    }
}

TString MakeUrl(TStringBuf host, ui16 port) {
    NUri::TUri uri(host, port, /* action = */ "/");
    return uri.PrintS();
}

TString MakeHostPort(TStringBuf host, ui16 port) {
    NUri::TUri uri(host, port, /* action = */ "/");
    return uri.PrintS(NUri::TField::FlagHostPort);
}

THashMap<TString, TString> MakeProxyHeaders(const IContext& ctx, TStringBuf requestId, TStringBuf guid) {
    THashMap<TString, TString> map;

    auto setWithLevels = [&map](TString key, TString value) {
        map[key] = value;
        for (int level = 0; level < 3; ++level) {
            key = TString::Join("x-yandex-proxy-header-", key);
            map[key] = value;
        }
    };

    // set X-YANDEX-JOKER
    TStringBuilder yandexJokerValue;
    yandexJokerValue << "prj=megamind";
    yandexJokerValue << "&sess=" << ctx.Config().JokerConfig().SessionId();
    yandexJokerValue << "&test=" << requestId;
    if (guid) {
        yandexJokerValue << "&group_id=" << guid;
    }
    setWithLevels("x-yandex-joker", yandexJokerValue);

    // set X-YANDEX-VIA-PROXY
    const auto& jokerSettings = ctx.JokerServerSettings();
    TString yandexViaProxyValue{MakeHostPort(jokerSettings->Host, jokerSettings->Port)};
    TString yandexJokerKey{"x-yandex-via-proxy"};
    setWithLevels("x-yandex-via-proxy", yandexViaProxyValue);

    // set X-YANDEX-VIA-PROXY-SKIP
    map["x-yandex-via-proxy-skip"] = "Vins Bass BassRun BassApply";

    return map;
}

TString BuildHeaders(const IContext& ctx, ui16 port, TStringBuf reqId, TStringBuf guid, TMaybe<TStringBuf> timestamp) {
    TStringBuilder headersBuilder;

    headersBuilder << "x-alice-client-reqid: " << reqId << "\r\n";
    headersBuilder << "X-RTLog-Token: " << TInstant::Now().MicroSeconds() << "$" << reqId << "$" << reqId << "\r\n";
    headersBuilder << "Host: " << MakeHostPort("localhost", port) << "\r\n";
    headersBuilder << "Connection: Keep-Alive" << "\r\n";

    if (timestamp) {
        TString key = "x-yandex-fake-time";
        headersBuilder << key << ": " << timestamp.GetRef() << "\r\n";
        for (int level = 0; level < 3; ++level) {
            key = TString::Join("x-yandex-proxy-header-", key);
            headersBuilder << key << ": " << timestamp.GetRef() << "\r\n";
        }
    }

    THashMap<TString, TString> proxyHeadersMap = MakeProxyHeaders(ctx, reqId, guid);
    for (const auto& p : proxyHeadersMap) {
        headersBuilder << p.first << ": " << p.second << "\r\n";
    }

    return headersBuilder;
}

TString BeautifyJson(TString json) {
    TStringStream ss(json);
    NJson::TJsonValue value = NJson::ReadJsonTree(&ss, /* throwOnError = */ true);
    return NJson::WriteJson(value, /* formatOutput = */ true, /* sortKeys = */ true);
}

} // namespace NAlice::NShooter
