#include "cache.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/network/headers.h>

#include <library/cpp/neh/neh.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/string/join.h>

namespace NBASS::NMusic {

namespace {

constexpr TStringBuf MOSCOW_IP = "77.88.55.77";

} // namespace

void TStationsData::Update() {
    NHttpFetcher::TRequestPtr req = YaRadioBackgroundSF.Request();
    req->AddHeader(NAlice::NNetwork::HEADER_X_FORWARDED_FOR, MOSCOW_IP);
    auto resp = req->Fetch()->Wait();
    if (!resp || resp->IsError()) {
        ythrow yexception() << TStringBuf("Radio stations fetching error: ")
                            << (resp ? resp->GetErrorText() : "no response");
    }

    NSc::TValue stations;
    if (!NSc::TValue::FromJson(stations, resp->Data) || !stations.Has("result")) {
        ythrow yexception() << "Radio stations update error: cannot parse JSON";
    }

    TStations newStations;
    for (const auto& st : stations.Get("result").GetArray()) {
        const NSc::TValue& stId = st.TrySelect("station/id");

        TStringBuf type = stId.Get("type").GetString();
        TStringBuf tag = stId.Get("tag").GetString();
        TString typeTag = Join('/', type, tag);
        TString name{st.TrySelect("station/name").GetString()};

        newStations[typeTag] = {name, typeTag};
        newStations[tag] = {name, typeTag};
    }

    LOG(DEBUG) << "Radio stations updated: total " << newStations.size() << " stations" << Endl;

    if (newStations.empty()) {
        ythrow yexception() << "Got an empty radio station list!";
    }

    Stations = std::move(newStations);
}

} // namespace NBASS::NMusic
