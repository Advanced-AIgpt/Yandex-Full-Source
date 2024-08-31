#pragma once

#include <alice/library/restriction_level/protos/content_settings.pb.h>

#include <alice/library/geo/geodb.h>
#include <alice/library/geo/user_location.h>
#include <alice/library/client/client_info.h>

#include <library/cpp/scheme/scheme.h>

#include <maps/doc/proto/yandex/maps/proto/common2/response.pb.h>

#include <util/generic/strbuf.h>

namespace NAlice {

    // [[deprecated("for backward compatibility with bass only")]]
    void ParseGeoObjectPage(const ::yandex::maps::proto::common2::response::Response& response,
        const NGeobase::TLookup& geobase, const TClientInfo& clientInfo,
        const TUserLocation& userLocation, EContentSettings restrictionLevel,
        const TStringBuf searchText, size_t resultIndexOnPage, size_t docsCount,
        TVector<NSc::TValue>& result, const TStringBuf privateNavigatorKey,
        bool supportsOpenLinkSearchViewport, bool supportsIntentUrls, NSc::TValue* resolvedWhere = nullptr);

    TVector<NSc::TValue> ParseGeoObjectInfos(const ::yandex::maps::proto::common2::response::Response& response,
        const NGeobase::TLookup& geobase, const TClientInfo& clientInfo,
        const TUserLocation& userLocation, EContentSettings restrictionLevel,
        TStringBuf searchText, size_t resultIndexOnPage, size_t docsCount,
        const TStringBuf privateNavigatorKey, bool supportsOpenLinkSearchViewport, bool supportsIntentUrls);

    NSc::TValue ParseResolvedWhere(const ::yandex::maps::proto::common2::response::Response& response,
        const NGeobase::TLookup& geobase, const TClientInfo& clientInfo);

} // namespace NAlice
