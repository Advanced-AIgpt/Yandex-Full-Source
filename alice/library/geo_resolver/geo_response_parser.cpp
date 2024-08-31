#include "geo_response_parser.h"

#include <alice/library/url_builder/url_builder.h>

#include <library/cpp/timezone_conversion/convert.h>

#include <maps/doc/proto/yandex/maps/proto/search/business_internal.pb.h>
#include <maps/doc/proto/yandex/maps/proto/search/business.pb.h>
#include <maps/doc/proto/yandex/maps/proto/search/experimental.pb.h>
#include <maps/doc/proto/yandex/maps/proto/search/geocoder_internal.pb.h>
#include <maps/doc/proto/yandex/maps/proto/search/geocoder.pb.h>
#include <maps/doc/proto/yandex/maps/proto/search/references.pb.h>
#include <maps/doc/proto/yandex/maps/proto/search/search.pb.h>

#include <util/datetime/base.h>
#include <util/stream/format.h>
#include <util/string/builder.h>

namespace NPbCommon = ::yandex::maps::proto::common2;
namespace NPbSearch = ::yandex::maps::proto::search;

namespace NAlice {

namespace {
    TString GetToponymUri(const TClientInfo& clientInfo, const TUserLocation& userLocation, const TStringBuf toponymName,
                          const NSc::TValue& location, const TStringBuf privateNavigatorKey, bool supportsIntentUrls) {
        TStringBuilder locationStr;
        locationStr << location["lon"].GetNumber() << ',' << location["lat"].GetNumber();
        TCgiParameters cgi;
        cgi.InsertUnescaped(TStringBuf("text"), toponymName);
        cgi.InsertEscaped(TStringBuf("ll"), locationStr);
        cgi.InsertEscaped(TStringBuf("oll"), locationStr);
        cgi.InsertUnescaped(TStringBuf("ol"), TStringBuf("geo"));
        return GenerateMapsUri(clientInfo, userLocation, "", cgi, privateNavigatorKey, supportsIntentUrls);
    }

    TString GetPoiUri(const TClientInfo& clientInfo, const TUserLocation& userLocation,
                      const TStringBuf searchText, const TStringBuf orgId, const NSc::TValue& location,
                      const TStringBuf privateNavigatorKey, bool supportsIntentUrls) {
        TStringBuilder locationStr;
        locationStr << location["lon"].GetNumber() << ',' << location["lat"].GetNumber();
        TCgiParameters cgi;
        cgi.InsertUnescaped(TStringBuf("text"), searchText);
        cgi.InsertUnescaped(TStringBuf("ll"), locationStr);
        cgi.InsertUnescaped(TStringBuf("ol"), TStringBuf("biz"));
        cgi.InsertUnescaped(TStringBuf("oid"), orgId);
        return GenerateMapsUri(clientInfo, userLocation, "", cgi, privateNavigatorKey, supportsIntentUrls);
    }

    TString GetAddressLine(const NSc::TValue& address, const TVector<TStringBuf>& addressFields) {
        const bool isPoi = (DetectGeoObjectType(address) == TStringBuf("poi"));
        const NSc::TValue& addressGeo = isPoi ? address["geo"] : address;

        TStringBuilder addressLine;
        for (const TStringBuf field : addressFields) {
            if (addressGeo.Has(field)) {
                addressLine << (addressLine ? ", " : "")
                            << addressGeo[field].GetString();
            }
        }
        return addressLine;
    }

    /** Makes address line
     * @param[in] address - NSc::TValue object with address fields ("country", "city", etc.)
     * @return addres line in "Country, City, Street, House" format
     */
    TString GetAddressLine(const NSc::TValue& address) {
        static const TVector<TStringBuf> addressFields = {"country", "city", "street", "house"};
        return GetAddressLine(address, addressFields);
    }

    /** Extract References info from geometasearch response
     * @param[in] metadata - references metadata
     * @param[out] references - NSc:::TValue object, filled with references data
     */
    void ExtractReferences(const NPbSearch::references::References& metadata, NSc::TValue* references) {
        (*references).Clear();
        for (const auto& reference : metadata.reference()) {
            (*references)[reference.scope()].SetString(reference.id());
        }
    }

    /** Extract address info from geometasearch response
     * @param[in] address - address info
     * @param[out] resultValue - NSc::TValue object, filled with address data
     */
    void ExtractAddressInfo(const NPbSearch::address::Address& address, NSc::TValue* resultValue) {
        using TKind = NPbSearch::kind::Kind;
        static const THashMap<TKind, TString> addressFields = {
            // https://tech.yandex.ru/maps/geocoder/doc/desc/reference/kind-docpage/
            {TKind::COUNTRY, "country"},
            {TKind::LOCALITY, "city"},
            {TKind::STREET, "street"},
            {TKind::AIRPORT, "airport"},
            {TKind::HOUSE, "house"},
            {TKind::METRO_STATION, "metro"},
            {TKind::RAILWAY_STATION, "railway"},
            {TKind::VEGETATION, "vegetation"},
            {TKind::OTHER, "other"},
        };

        for (const auto& component : address.component()) {
            if (!component.kind().empty()) {
                const auto& kind = component.kind(0);
                if (const auto* kindStr = addressFields.FindPtr(kind)) {
                    (*resultValue)[*kindStr] = component.name();
                }
            }
        }
        if (address.formatted_address()) {
            (*resultValue)["address_line"].SetString(address.formatted_address());
        } else {
            // Try to build address_line, if it's empty (for instance, for countries geometasearch returns empty address_line)
            TString addressLine = GetAddressLine(*resultValue);
            (*resultValue)["address_line"] = addressLine;
        }
    }

    void GetHoursFromTo(const NPbSearch::hours::Hours& hours, TVector<std::pair<TString, TString>>& working, bool& allDay) {
        const auto timeToString = [](ui32 time) {
            return TStringBuilder() << LeftPad(time / 60 / 60, 2, '0') << ":" << LeftPad(time / 60 % 60, 2, '0') << ":00";
        };

        allDay = AnyOf(hours.time_range(), [](const auto& timeRange) {
                return timeRange.all_day();
        });

        if (allDay) {
            working.emplace_back("00:00:00", "23:59:59");
        } else {
            for (const auto& timeRange : hours.time_range()) {
                auto& [from, to] = working.emplace_back();
                if (timeRange.has_from()) {
                    from = timeToString(timeRange.from());
                }
                if (timeRange.has_to()) {
                    to = timeToString(timeRange.to());
                }
            }
        }
    }

    void FillHoursInfo(const NGeobase::TLookup& geobase, const TClientInfo& clientInfo, const TUserLocation& location,
                       const NPbSearch::hours::OpenHours& openHours, const TMaybe<NGeobase::TId>& geoid, NSc::TValue& hoursInfo)
    {
        using TDayOfWeek = NPbSearch::hours::DayOfWeek;
        static const TVector<TDayOfWeek> weekDays = {
            TDayOfWeek::SUNDAY, TDayOfWeek::MONDAY, TDayOfWeek::TUESDAY, TDayOfWeek::WEDNESDAY, TDayOfWeek::THURSDAY, TDayOfWeek::FRIDAY, TDayOfWeek::SATURDAY,
        };

        hoursInfo.Clear();
        NSc::TValue& poiTimezone = hoursInfo.Add("tz");
        poiTimezone.SetNull();
        if (geoid && IsValidId(*geoid)) {
            auto timezone = geobase.GetTimezoneName(*geoid);
            if (!timezone.empty()) {
                poiTimezone.SetString(timezone);
            }
        }

        const bool hasPoiTimezone = !poiTimezone.IsNull();

        // need timezone to select weekday
        // use organization timezone or user timezone instead
        const NDatetime::TTimeZone tz = NDatetime::GetTimeZone(hasPoiTimezone ? poiTimezone.GetString() : location.UserTimeZone());
        const NDatetime::TSimpleTM curTime = NDatetime::ToCivilTime(TInstant::Seconds(clientInfo.Epoch), tz);

        const TDayOfWeek curWeekDay = weekDays.at(curTime.WDay);
        const ui8 prevWDay = curTime.WDay ? curTime.WDay - 1 : 6;
        const TDayOfWeek prevWeekDay = weekDays.at(prevWDay);

        TString yesterdayFrom, yesterdayTo;
        TVector<std::pair<TString, TString>> todayWorking;
        TVector<std::pair<TString, TString>> yesterdayWorking;

        bool today24 = false, yesterday24 = false;
        for (const auto& hours : openHours.hours()) {
            if (FindPtr(hours.day(), TDayOfWeek::EVERYDAY)) {
                GetHoursFromTo(hours, todayWorking, today24);
                yesterdayFrom = todayWorking.back().first;
                yesterdayTo = todayWorking.back().second;
                yesterday24 = today24;
            } else {
                if (FindPtr(hours.day(), curWeekDay)) {
                    GetHoursFromTo(hours, todayWorking, today24);
                }
                if (FindPtr(hours.day(), prevWeekDay)) {
                    GetHoursFromTo(hours, yesterdayWorking, yesterday24);
                    yesterdayFrom = yesterdayWorking.back().first;
                    yesterdayTo = yesterdayWorking.back().second;
                }
            }
        }

        if (today24) {
            hoursInfo["24_hours"].SetBool(true);
            hoursInfo["current_status"] = "open";
        } else {
            const TString curTimeStr = curTime.ToString("%T"); // %H:%M:%S
            NSc::TValue& workingHours = hoursInfo["working"].SetArray();

            bool openByYesterdayTime = hasPoiTimezone && (yesterdayTo < yesterdayFrom) && (curTimeStr <= yesterdayTo);
            if (openByYesterdayTime) {
                NSc::TValue& interval = workingHours.Push();
                interval["from"] = yesterdayFrom;
                interval["to"] = yesterdayTo;
            }

            bool openByTodayTime = false;
            for (const auto& todayInterval : todayWorking) {
                const TString& todayFrom = todayInterval.first;
                const TString& todayTo = todayInterval.second;

                NSc::TValue& interval = workingHours.Push();
                interval["from"] = todayFrom;
                interval["to"] = todayTo;

                if (hasPoiTimezone) {
                    const TStringBuf todayToFixed = (todayTo >= todayFrom) ? todayTo : TStringBuf("23:59:59");
                    openByTodayTime = openByTodayTime || (curTimeStr >= todayFrom && curTimeStr <= todayToFixed);
                }
            }

            if (hoursInfo["working"].ArrayEmpty()) {
                // Vins requires nulls, not empty strings
                hoursInfo["working"].SetNull();
            }
            if (hasPoiTimezone) {
                hoursInfo["current_status"] = (openByYesterdayTime || openByTodayTime) ? "open" : "closed";
            }
        }
    }

    /** Extract POI info from geometasearch response
     * @param[in] ctx - context object (needed to know, how to format phone and object uris, etc.)
     * @param[in] metadata - business metadata
     * @param[out] resultPoi - NSc::TValue object, filled with POI data
     */
    void ExtractPoiInfo(const TClientInfo& clientInfo, const NGeobase::TLookup& geobase,
                        const TUserLocation& userLocation, EContentSettings restrictionLevel,
                        const NPbSearch::business::GeoObjectMetadata& metadata, bool canOpenLinkSearchViewport,
                        NSc::TValue* resultPoi) {
        (*resultPoi).Clear();

        (*resultPoi)["object_id"].SetString(metadata.id());
        (*resultPoi)["company_name"].SetString(metadata.name());
        (*resultPoi)["name"].SetString(metadata.short_name() ? metadata.short_name() : metadata.name());

        for (const auto& link : metadata.link()) {
            if (link.type() == link.SELF) {
                (*resultPoi)["url"].SetString(link.link().href());
                break;
            }
        }

        const bool isTouchOrChatBot = clientInfo.IsTouch() || clientInfo.IsChatBot();
        if (!metadata.phone().empty() && isTouchOrChatBot) {
            const auto& phone = *metadata.phone().begin();
            (*resultPoi)["phone"].SetString(phone.formatted());
            (*resultPoi)["phone_uri"].SetString(
                GeneratePhoneUri(clientInfo, phone.formatted())
            );
        }

        NSc::TValue& resultPoiGeo = (*resultPoi)["geo"];
        ExtractAddressInfo(metadata.address(), &resultPoiGeo);
        resultPoiGeo["level"] = "inside_city";

        TString seoname;
        TMaybe<NGeobase::TId> geoid;
        if (metadata.HasExtension(NPbSearch::business_internal::COMPANY_INFO)) {
            const auto& companyInfo = metadata.GetExtension(NPbSearch::business_internal::COMPANY_INFO);
            geoid = companyInfo.geoid();
            if (companyInfo.seoname()) {
                seoname = companyInfo.seoname();
            }
        }

        if (geoid) {
            NGeobase::TId cityGeoId = ReduceGeoIdToCity(geobase, *geoid);
            resultPoiGeo["geoid"].SetNumber(cityGeoId);
            FillCityPrepcase(geobase, cityGeoId, clientInfo.Locale.Lang, &resultPoiGeo);
        }

        const auto& objectName = metadata.name();
        const auto& objectId = metadata.id();
        (*resultPoi)["object_uri"].SetString(
            GenerateObjectUri(clientInfo, userLocation, restrictionLevel, objectId, objectName, seoname, canOpenLinkSearchViewport)
        );
        (*resultPoi)["object_catalog_uri"].SetString(
            GenerateObjectCatalogUri(userLocation, objectId)
        );
        (*resultPoi)["object_catalog_reviews_uri"].SetString(
            GenerateObjectCatalogUri(userLocation, objectId, true /* reviewsIntent */)
        );

        (*resultPoi)["object_catalog_photos_uri"].SetString(
            GenerateObjectCatalogUri(userLocation, objectId, false, /* reviewsIntent */ true /* photoIntent */)
        );

        if (metadata.has_open_hours()) {
            FillHoursInfo(geobase, clientInfo, userLocation, metadata.open_hours(), geoid, (*resultPoi)["hours"]);
        }
    }

    /** Extract toponym info from geometasearch response
     * @param[in] ctx - context object (needed to know lang, etc.)
     * @param[in] metadata - geocoder metadata
     * @param[out] resultGeo - NSc::TValue object, filled with toponym data
     */
    void ExtractGeoInfo(const NGeobase::TLookup& geobase, const TString& lang, const NPbSearch::geocoder::GeoObjectMetadata& metadata, NSc::TValue* resultGeo) {
        (*resultGeo).Clear();

        ExtractAddressInfo(metadata.address(), resultGeo);

        if (metadata.HasExtension(NPbSearch::geocoder_internal::TOPONYM_INFO)) {
            const auto& toponymInfo = metadata.GetExtension(NPbSearch::geocoder_internal::TOPONYM_INFO);

            NGeobase::TId cityGeoId = ReduceGeoIdToCity(geobase, toponymInfo.geoid());
            (*resultGeo)["geoid"].SetNumber(cityGeoId);
            FillCityPrepcase(geobase, cityGeoId, lang, resultGeo);

            (*resultGeo)["location"]["lon"] = toponymInfo.point().lon();
            (*resultGeo)["location"]["lat"] = toponymInfo.point().lat();
        }

        NSc::TValue& geoLevel = (*resultGeo).Add("level");
        if (metadata.has_address() && !metadata.address().component().empty()) {
            const auto& lastComponent = *metadata.address().component().rbegin();
            if (!lastComponent.kind().empty() && lastComponent.kind(0) == NPbSearch::kind::LOCALITY) {
                geoLevel.SetString("city");
            } else {
                if (!(*resultGeo).Has("city")) {
                    geoLevel.SetString("over_city");
                } else if ((*resultGeo).TrySelect("address_line").GetString() == (*resultGeo)["city"]) {
                    geoLevel.SetString("city");
                } else {
                    geoLevel.SetString("level") = "inside_city";
                }
            }
        }
    }

    void ExtractGeoObjectInfo(const NGeobase::TLookup& geobase, const TClientInfo& clientInfo,
                              const TUserLocation& userLocation, EContentSettings restrictionLevel, const TStringBuf searchText,
                              const NPbCommon::geo_object::GeoObject& geoObject, NSc::TValue* resultObject, const TStringBuf privateNavigatorKey,
                              bool supportsOpenLinkSearchViewport, bool supportsIntentUrls) {
        (*resultObject).Clear();

        float accuracy = 0;
        NSc::TValue location = NSc::Null();
        if (!geoObject.geometry().empty()) {
            for (const auto& geometry : geoObject.geometry()) {
                if (geometry.has_point()) {
                    location["lon"] = geometry.point().lon();
                    location["lat"] = geometry.point().lat();
                    break;
                }
            }
        }

        bool isPoi = false;
        NSc::TValue geoData, poiData, references, subtitles;
        for (const auto& commonMetadata : geoObject.metadata()) {
            if (commonMetadata.HasExtension(NPbSearch::business::GEO_OBJECT_METADATA)) {
                const auto& metadata = commonMetadata.GetExtension(NPbSearch::business::GEO_OBJECT_METADATA);
                ExtractPoiInfo(clientInfo, geobase, userLocation, restrictionLevel, metadata, supportsOpenLinkSearchViewport, &poiData);
                isPoi = true;
            }
            if (commonMetadata.HasExtension(NPbSearch::experimental::GEO_OBJECT_METADATA)) {
                const auto& metadata = commonMetadata.GetExtension(NPbSearch::experimental::GEO_OBJECT_METADATA);
                for (const auto& item : metadata.experimental_storage().item()) {
                    if (item.key() == "accuracy") {
                        TryFromString(item.value(), accuracy);
                    } else if (item.key() == "matchedobjects/1.x") {
                        NSc::TValue::FromJson(subtitles, item.value());
                    }
                }
            }
            if (commonMetadata.HasExtension(NPbSearch::geocoder::GEO_OBJECT_METADATA)) {
                const auto& metadata = commonMetadata.GetExtension(NPbSearch::geocoder::GEO_OBJECT_METADATA);
                ExtractGeoInfo(geobase, clientInfo.Locale.Lang, metadata, &geoData);
            }
            if (commonMetadata.HasExtension(NPbSearch::references::GEO_OBJECT_METADATA)) {
                const auto& metadata = commonMetadata.GetExtension(NPbSearch::references::GEO_OBJECT_METADATA);
                ExtractReferences(metadata, &references);
            }
        }

        if (isPoi) {
            if (!poiData.IsNull()) {
                poiData["geo_uri"] = GetPoiUri(clientInfo, userLocation, searchText, poiData["object_id"], location,
                    privateNavigatorKey, supportsIntentUrls);
                if (!references.IsNull()) {
                    poiData["references"] = references;
                }
                if (!subtitles.IsNull()) {
                    poiData["subtitles"] = subtitles;
                }
                if (!location.IsNull()) {
                    poiData["location"] = location;
                }
            }
            *resultObject = poiData;
        } else {
            // Accuracy is returned for toponyms. If it's less 1, then we treat it as a trash
            if (accuracy < 1) {
                //LOG(DEBUG) << TStringBuf("bad toponym found for ") << SearchText << Endl;
                (*resultObject).SetNull();
            } else {
                if (!geoData.IsNull()) {
                    geoData["geo_uri"] = GetToponymUri(clientInfo, userLocation, geoData["address_line"].GetString(),
                        location, privateNavigatorKey, supportsIntentUrls);
                }
                *resultObject = geoData;
            }
        }
    }

void ParseGeoObjectInfos(
    const NPbCommon::response::Response& response,
    const NGeobase::TLookup& geobase, const TClientInfo& clientInfo,
    const TUserLocation& userLocation, EContentSettings restrictionLevel,
    TStringBuf searchText, size_t resultIndexOnPage, size_t docsCount,
    TVector<NSc::TValue>& result, const TStringBuf privateNavigatorKey,
    bool supportsOpenLinkSearchViewport, bool supportsIntentUrls)
{
    if (!response.has_reply()) {
        return;
    }

    size_t docIndex = 0;
    for (const auto& object : response.reply().geo_object()) {
        if (result.size() == docsCount) {
            break;
        }

        ++docIndex;
        if (docIndex >= resultIndexOnPage) {
            result.emplace_back();
            ExtractGeoObjectInfo(geobase, clientInfo, userLocation, restrictionLevel, searchText, object, &result.back(),
                                 privateNavigatorKey, supportsOpenLinkSearchViewport, supportsIntentUrls);
        }
    }
}

void ParseResolvedWhere(const NPbCommon::response::Response& response,
    const NGeobase::TLookup& geobase, const TClientInfo& clientInfo,
    NSc::TValue* resolvedWhere)
{
    if (!resolvedWhere || !response.has_reply()) {
        return;
    }

    for (const auto& replyMetadata : response.reply().metadata()) {
        if (replyMetadata.HasExtension(NPbSearch::search::SEARCH_RESPONSE_METADATA)) {
            const auto& searchMetadata = replyMetadata.GetExtension(NPbSearch::search::SEARCH_RESPONSE_METADATA);

            for (const auto& commonMetadata : searchMetadata.geo_object().metadata()) {
                if (commonMetadata.HasExtension(NPbSearch::geocoder::GEO_OBJECT_METADATA)) {
                    const auto& metadata = commonMetadata.GetExtension(NPbSearch::geocoder::GEO_OBJECT_METADATA);
                    ExtractGeoInfo(geobase, clientInfo.Locale.Lang, metadata, resolvedWhere);
                    break;
                }
            }

            break;
        }
    }
}

} // namespace

void ParseGeoObjectPage(const NPbCommon::response::Response& response,
    const NGeobase::TLookup& geobase, const TClientInfo& clientInfo,
    const TUserLocation& userLocation, EContentSettings restrictionLevel,
    TStringBuf searchText, size_t resultIndexOnPage, size_t docsCount,
    TVector<NSc::TValue>& result, const TStringBuf privateNavigatorKey,
    bool supportsOpenLinkSearchViewport, bool supportsIntentUrls, NSc::TValue* resolvedWhere)
{
    ParseGeoObjectInfos(response, geobase, clientInfo,
        userLocation, restrictionLevel, searchText, resultIndexOnPage, docsCount, result, privateNavigatorKey,
        supportsOpenLinkSearchViewport, supportsIntentUrls);
    ParseResolvedWhere(response, geobase, clientInfo, resolvedWhere);
}

TVector<NSc::TValue> ParseGeoObjectInfos(const NPbCommon::response::Response& response,
    const NGeobase::TLookup& geobase, const TClientInfo& clientInfo,
    const TUserLocation& userLocation, EContentSettings restrictionLevel,
    TStringBuf searchText, size_t resultIndexOnPage, size_t docsCount,
    const TStringBuf privateNavigatorKey, bool supportsOpenLinkSearchViewport, bool supportsIntentUrls)
{
    TVector<NSc::TValue> result;
    ParseGeoObjectInfos(response, geobase, clientInfo,
        userLocation, restrictionLevel, searchText, resultIndexOnPage, docsCount, result, privateNavigatorKey,
        supportsOpenLinkSearchViewport, supportsIntentUrls);
    return result;
}

NSc::TValue ParseResolvedWhere(const NPbCommon::response::Response& response,
    const NGeobase::TLookup& geobase, const TClientInfo& clientInfo)
{
    NSc::TValue result;
    ParseResolvedWhere(response, geobase, clientInfo, &result);
    return result;
}

} // namespace NAlice
