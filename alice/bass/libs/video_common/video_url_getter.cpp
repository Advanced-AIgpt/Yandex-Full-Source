#include "video_url_getter.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/video_common/defs.h>

#include <library/cpp/scheme/scheme.h>

#include <util/string/builder.h>

namespace NVideoCommon {

TVideoUrlGetter::TVideoUrlGetter(const TParams& params)
    : UseZora(params.UseZora)
{
    Options.Timeout = params.Timeout;
    Options.RetryPeriod = params.RetryPeriod;
    Options.MaxAttempts = params.MaxAttempts;
}

TMaybe<TString> TVideoUrlGetter::Get(const TRequest& request) const {
    if (!request.ProviderName) {
        LOG(ERR) << "Empty provider name" << Endl;
        return Nothing();
    }
    TStringBuf providerName = *request.ProviderName;

    if (providerName == PROVIDER_YOUTUBE) {
        if (!request.ProviderItemId) {
            LOG(ERR) << "Empty ProviderItemId" << Endl;
            return Nothing();
        }
        return TStringBuilder() << "http://www.youtube.com/watch?v=" << *request.ProviderItemId;
    }

    if (providerName == PROVIDER_KINOPOISK) {
        if (!request.KinopoiskId) {
            LOG(ERR) << "Empty kinopoisk id" << Endl;
            return Nothing();
        }
        return TStringBuilder() << "https://www.kinopoisk.ru/film/" << *request.KinopoiskId;
    }

    if (providerName == PROVIDER_IVI) {
        if (!request.Type || !request.ProviderItemId) {
            LOG(ERR) << "Empty Type or ProviderItemId" << Endl;
            return Nothing();
        }
        TStringBuf type = *request.Type;
        TStringBuf providerItemId = *request.ProviderItemId;
        if (type == "movie")
            return TStringBuilder() << "http://www.ivi.ru/watch/" << providerItemId << "/description";
        if (type == "tv_show" || type == "tv_show_episode") {
            const TString url = TStringBuilder() << "https://api.ivi.ru/mobileapi/compilationinfo/v5/?id="
                                                 << providerItemId;
            const auto response = MakeRequest(url);
            if (response->IsError()) {
                LOG(ERR) << TStringBuilder() << "IVI request error: " << response->GetErrorText() << Endl;
                return Nothing();
            }
            NSc::TValue json = NSc::TValue::FromJson(response->Data);
            const TStringBuf hru = json["result"]["hru"].GetString();
            return TStringBuilder() << "http://www.ivi.ru/watch/" << hru;
        }
        LOG(ERR) << TStringBuilder() << "Unsupported IVI item type " << type << Endl;
        return Nothing();
    }

    if (providerName == PROVIDER_AMEDIATEKA) {
        if (!request.Type || !request.ProviderItemId) {
            LOG(ERR) << "Empty Type or ProviderItemId" << Endl;
            return Nothing();
        }
        TStringBuf handler;
        TStringBuf path;
        TStringBuf id = *request.ProviderItemId;
        TStringBuf type = *request.Type;
        if (type == "movie") {
            handler = TStringBuf("films");
            path = TStringBuf("film");
        } else if (type == "tv_show" || type == "tv_show_episode") {
            handler = TStringBuf("serials");
            path = TStringBuf("serial");
            if (type == "tv_show_episode") {
                if (!request.TvShowItemId) {
                    LOG(ERR) << "Empty TvShowItemId" << Endl;
                    return Nothing();
                }
                id = *request.TvShowItemId;
            }
        } else {
            LOG(ERR) << TStringBuilder() << "Unsupported AMEDIATEKA item type " << type << Endl;
            return Nothing();
        }
        const TString url = TStringBuilder() << "https://api.amediateka.ru/v1/" << handler << '/' << id << ".json"
                                             << "/?client_id=amediateka";
        const auto response = MakeRequest(url);
        if (response->IsError()) {
            LOG(ERR) << TStringBuilder() << "AMEDIATEKA request error: " << response->GetErrorText() << Endl;
            return Nothing();
        }
        NSc::TValue json = NSc::TValue::FromJson(response->Data);
        const TStringBuf slug = json[path]["slug"].GetString();
        return TStringBuilder() << "http://www.amediateka.ru/" << path << '/' << slug;
    }

    TStringBuilder msg = TStringBuilder{} << "Unknown provider " << providerName;
    if (request.Type)
        msg << " with type " << *request.Type;
    LOG(ERR) << msg << Endl;
    return Nothing();
}

NHttpFetcher::TResponse::TRef TVideoUrlGetter::MakeRequest(TStringBuf url) const {
    NHttpFetcher::TRequestPtr request = NHttpFetcher::Request(url, Options);
    if (UseZora) {
        // TODO create helper for setting zora as proxy
        request->SetProxy("zora.yandex.net:8166");
        request->AddHeader("X-Yandex-Requesttype", "userproxy");
        request->AddHeader("X-Yandex-Sourcename", "bass");

        if (request->Url().StartsWith(TStringBuf("https:")))
            request->AddHeader("X-Yandex-Use-Https", "1");
    }
    return request->Fetch()->Wait();
}

} // namespace NVideoCommon
