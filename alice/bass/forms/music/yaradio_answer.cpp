#include "answers.h"

#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/join.h>

namespace NBASS::NMusic {

namespace {

constexpr TStringBuf ONYOURWAVE = "user/onyourwave";

} // namespace

void TYandexRadioAnswer::InitCommonStation(const TString& stationTag) {
    LOG(DEBUG) << "InitCommonStation" << Endl;
    Station = RadioStations.GetStation(stationTag);
}

void TYandexRadioAnswer::InitPersonalStation(const NSc::TValue& status) {
    if (!status.IsNull() && status.Get("stationExists").GetBool(false)) {
        TStringBuf login = status.TrySelect("account/login").GetString();
        TStringBuf stationName = status.TrySelect("stationData/name").GetString();
        Station.Tag = TString{"user/"} + login;
        Station.Name = stationName == login ? TString{} : stationName;

    } else {
        Station.Tag = ONYOURWAVE;
        Station.Name = RadioStations.GetStation(ONYOURWAVE).Name;
    }

    IsPersonal = true;
}

bool TYandexRadioAnswer::ConvertAnswerToOutputFormat(NSc::TValue* value) {
    if (!IsPersonal && Station.Name.empty()) {
        return false;
    }
    (*value)["type"].SetString("radio");
    (*value)["station"]["tag"] = Station.Tag;
    (*value)["station"]["name"] = Station.Name;
    if (IsPersonal) {
        (*value)["station"]["is_personal"] = IsPersonal;
    }
    (*value)["uri"] = MakeLink();
    return true;
}

TString TYandexRadioAnswer::MakeLink() const {
    if (Station.Tag.empty()) {
        return TString();
    }
    TCgiParameters cgi;
    if (!ClientFeatures.IsSmartSpeaker()) {
        cgi.InsertEscaped(TStringBuf("play"), TStringBuf("true"));
    }
    return GenerateMusicAppUri(ClientFeatures, IsPersonal ? EMusicUriType::RadioPersonal : EMusicUriType::Radio, Station.Tag, cgi);
}

bool TYandexRadioAnswer::MakeOutputFromServiceData(const NAlice::TClientFeatures& clientFeatures, const NSc::TValue& answer, NSc::TValue* output) {
    if (!answer.Has("station")) {
        return false;
    }

    const NSc::TValue& station = answer.Get("station");
    if (!station.IsDict()) {
        return false;
    }
    TStringBuilder tag;
    TStringBuf type = station.TrySelect("id/type").GetString();

    tag << type << '/' << station.TrySelect("id/tag").GetString();
    if (tag == TStringBuf("/")) {
        return false;
    }

    (*output)["type"].SetString("radio");
    (*output)["station"]["tag"] = tag;

    // FIXME: Personal station name
    bool isPersonal = false;
    if (type == TStringBuf("user")) {
        (*output)["station"]["is_personal"] = true;
        (*output)["station"]["name"] = TStringBuf("Персональное");
        isPersonal = true;
    } else {
        (*output)["station"]["name"] = station.Get("name").GetString();
    }

    TCgiParameters cgi;
    if (!clientFeatures.IsSmartSpeaker()) {
        cgi.InsertEscaped(TStringBuf("play"), TStringBuf("true"));
    }

    (*output)["uri"] = GenerateMusicAppUri(clientFeatures, isPersonal ? EMusicUriType::RadioPersonal : EMusicUriType::Radio, tag, cgi);
    return true;
}

} // namespace NBASS::NMusic
