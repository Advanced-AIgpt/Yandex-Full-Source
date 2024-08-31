#include "providers.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/util/system_time.h>

namespace NBASS::NMusic {

namespace {

constexpr size_t NUM_SUGGESTED_STATIONS = 3;

const TStringBuf RADIO_ACCOUNT_PATH = "/account/status";
const TStringBuf RADIO_DASHBOARD_PATH = "/stations/dashboard";

}

TYandexRadioProvider::TYandexRadioProvider(TContext& ctx)
    : TBaseMusicProvider(ctx)
    , ServiceAnswer(ctx.ClientFeatures(), ctx.GlobalCtx().RadioStations())
{
}

bool TYandexRadioProvider::InitRequestParams(const NSc::TValue& slotData) {
    LOG(DEBUG) << "TYandexRadioProvider::InitRequestParams called" << Endl;
    if (DataGetHasKey(slotData, MAIN_FILTERS_SLOTS)) {
        for (const auto& filter : SLOT_NAMES.find(MAIN_FILTERS_SLOTS)->second) {
            if (slotData[MAIN_FILTERS_SLOTS].Get(filter).GetArray().size() > 0) {
                Station = slotData[MAIN_FILTERS_SLOTS][filter][0].GetString();
                if (filter == TStringBuf("language")) {
                    if (Station == TStringBuf("russian")) {
                        Station = TStringBuf("ruspop");
                    } else {
                        Station = TStringBuf("the-greatest-hits");
                    }
                }
                break;
            }
        }
    }

    return true;
}

TResultValue TYandexRadioProvider::Apply(NSc::TValue* out,
                                         NSc::TValue&& /* applyArguments */) {
    if (Station.empty()) {
        if (Ctx.UserAuthorizationHeader().empty()) {
            ServiceAnswer.InitPersonalStation();
        } else {
            NHttpFetcher::THandle::TRef handler = CreateRequestHandler(Ctx.GetSources().YaRadioAccount(RADIO_ACCOUNT_PATH), TCgiParameters());
            if (!handler) {
                return TError(
                    TError::EType::SYSTEM,
                    TStringBuf("cannot_create_request")
                );
            }

            NSc::TValue answer;
            const auto startMillis = NAlice::SystemTimeNowMillis();
            TResultValue reqError = WaitAndParseResponse(handler, &answer);
            Ctx.AddStatsCounter("YaRadioAccount_account_status_milliseconds", NAlice::SystemTimeNowMillis() - startMillis);
            if (reqError) {
                if (reqError->Type != TError::EType::UNAUTHORIZED) {
                    return reqError;
                }
                Ctx.AddAttention(TStringBuf("music_authorization_problem"), NSc::TValue());
                ServiceAnswer.InitPersonalStation();
            } else {
                ServiceAnswer.InitPersonalStation(answer.Get("result"));
            }
        }
    } else {
        ServiceAnswer.InitCommonStation(Station);
    }

    if (!ServiceAnswer.ConvertAnswerToOutputFormat(out)) {
        return TError(TError::EType::MUSICERROR, ERROR_MUSIC_NOT_FOUND);
    }
    return TResultValue();
}

void TYandexRadioProvider::AddSuggest(const NSc::TValue& result) {
    TCgiParameters cgi;
    cgi.InsertEscaped(TStringBuf("limit"), ToString(NUM_SUGGESTED_STATIONS + 1));

    NHttpFetcher::THandle::TRef handler = CreateRequestHandler(Ctx.GetSources().YaRadioDashboard(RADIO_DASHBOARD_PATH), cgi);
    if (!handler) {
        return;
    }
    const auto startMillis = NAlice::SystemTimeNowMillis();
    NHttpFetcher::TResponse::TRef resp = handler->Wait();
    Ctx.AddStatsCounter("YaRadioDashboard_stations_dashboard_milliseconds", NAlice::SystemTimeNowMillis() - startMillis);

    NSc::TValue answer;
    if (TResultValue reqError = ParseProviderResponse(resp, &answer)) {
        LOG(ERR) << "yaradio error : " << reqError->Msg << Endl;
        return;
    }

    const NSc::TValue& stations = answer.TrySelect("result/stations");
    if (!stations.IsArray()) {
        return;
    }

    size_t counter = 0;
    for (const auto& st : stations.GetArray()) {
        NSc::TValue suggest;
        if (!TYandexRadioAnswer::MakeOutputFromServiceData(Ctx.ClientFeatures(), st, &suggest)) {
            continue;
        }

        if (suggest.TrySelect("station/tag").GetString() == result.TrySelect("station/tag")) {
            continue;
        }

        NSc::TValue formUpdate;
        formUpdate["name"] = MUSIC_PLAY_FORM_NAME;
        formUpdate["resubmit"].SetBool(true);
        NSc::TValue& slot = formUpdate["slots"].SetArray().Push();
        slot["name"].SetString(TStringBuf("answer"));
        slot["type"].SetString(TStringBuf("music_result"));
        slot["optional"].SetBool(true);
        slot["value"] = suggest;
        Ctx.AddSuggest(TStringBuf("music__suggest_radio"), suggest, formUpdate);

        ++counter;
        if (counter == NUM_SUGGESTED_STATIONS) {
            break;
        }
    }
}

} // namespace NBASS:NMusic
