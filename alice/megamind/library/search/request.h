#pragma once

#include "search.h"

#include <alice/bass/libs/fetcher/request.h>
#include <alice/library/network/request_builder.h>
#include <alice/megamind/library/speechkit/request.h>
#include <alice/megamind/library/sources/request.h>

#include <alice/library/restriction_level/protos/content_settings.pb.h>

#include <alice/library/logger/logger.h>

#include <library/cpp/geobase/lookup.hpp>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice {

class TWebSearchBuilder;

// TODO (petrk) Merge it with alice/library/websearch.
class TWebSearchRequestBuilder {
public:
    explicit TWebSearchRequestBuilder(const TString& text);

    TWebSearchRequestBuilder& UpdateText(const TString& text);

    /** Set user ticket.
     * if the ticket is empty it will clear internal data.
     */
    TWebSearchRequestBuilder& SetUserTicket(const TString& userTicket);

    /** Set uid.
    * if the uid is empty it will clear internal data.
    */
    TWebSearchRequestBuilder& SetUid(const TString& uid);

    TWebSearchRequestBuilder& SetQuotaName(const TString& quotaName);
    TWebSearchRequestBuilder& SetUserRegion(NGeobase::TId lr);
    TWebSearchRequestBuilder& SetContentSettings(EContentSettings contentSettings);
    TWebSearchRequestBuilder& SetSensors(NMetrics::ISensors& sensors);
    TWebSearchRequestBuilder& SetHasImageSearchGranet();

    TSourcePrepareStatus Build(const TSpeechKitRequest& skr, const IEvent& event,
                               TRTLogger& logger, NNetwork::IRequestBuilder& request) const;

private:
    TSourcePrepareStatus Build(const TSpeechKitRequest& skr, const IEvent& event,
                               TRTLogger& logger, TWebSearchBuilder& webSearchBuilder) const;

    TString Text_;
    TString UseQuota_;
    TMaybe<NGeobase::TId> Lr_;
    TMaybe<EContentSettings> ContentSettings_;
    TMaybe<TString> UserTicket_;
    TMaybe<TString> Uid_;
    NMetrics::ISensors* Sensors_;
    bool HasImageSearchGranet_;
};

} // namespace NAlice
