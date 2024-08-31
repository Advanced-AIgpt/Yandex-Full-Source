#pragma once
#include "websearch.h"

#include <alice/library/client/client_features.h>
#include <alice/library/client/client_info.h>
#include <alice/library/restriction_level/restriction_level.h>

#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/geobase/lookup.hpp>

#include <util/generic/fwd.h>

namespace NAlice {


NAlice::TWebSearchBuilder PrepareSearchRequest(
    const TStringBuf query,
    const TClientInfo& clientInfo,
    const THashMap<TString, TMaybe<TString>>& experiments,
    bool canOpenLink,
    const TMaybe<NAlice::TEvent>& speechkitEvent,
    const TString& userAgent,
    const EContentRestrictionLevel contentRestrictionLevel,
    const TString& formName,
    const TMaybe<TStringBuf> lang,
    const TCgiParameters& cgi,
    const TStringBuf reqId,
    const TMaybe<TStringBuf> uuid,
    const TMaybe<TStringBuf> userTicket,
    const TMaybe<TString>& yandexUid,
    const TMaybe<TStringBuf> userIp,
    const TVector<TString>& cookies,
    const NAlice::TWebSearchBuilder::EService service,
    const TStringBuf megamindCgiString,
    const TMaybe<TStringBuf> processId,
    const TStringBuf rngSeed,
    NGeobase::TId lr,
    const bool hasImageSearchGranet,
    const TStringBuf hamsterQuota,
    const bool waitAll,
    // output params
    TString& encodedAliceMeta,
    std::function<void(const TStringBuf)> logger
);

} // namespace NAlice
