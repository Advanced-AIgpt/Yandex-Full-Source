#pragma once

#include <alice/library/geo_resolver/geo_response_parser.h>
#include <alice/library/geo_resolver/geo_position.h>

#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/util/error.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/xml/document/xml-document.h>

#include <search/idl/meta.pb.h>

#include <util/string/builder.h>

namespace NBASS {

static constexpr TStringBuf GEOSEARCH_ORIGIN = "assistant_bass";

using TGeoPosition = NAlice::TGeoPosition;

TMaybe<TGeoPosition> InitGeoPositionFromLocation(TContext::TMeta::TLocationConst location);

class TGeoObjectResolver {
public:
    /** Create resolver object and initialize async request to geometasearch and
      *   optionally to upper search.
      * @param[in] ctx - context object (used to get data about user's location)
      * @param[in] searchText - textual representation of a place for resolving
      * @param[in] searchPos - search position (where user is located, or where the results must be)
      * @param[in] geoType - list of geo types, separated by comma (default value: "geo,biz")
      * @param[in] sortBy - sort mode (empty by default, which means sorting by relevance)
      * @param[in] businessFilters - vector of string-valued business filters, like "open_24h:1"
      * @param[in] resultIndex - needed index of result (starts from 1, default value is 1)
      * @param[in] includeExperimentalMeta - asks for experimental meta information
      * @param[in] checkReturningSingleElement - asks upper search whether we need to show just one element.
      *   If this flag is set and searsh answers "yes", then WaitAndParseResponse (the version with docsCount)
      *   returns no more than one element.
      */
    TGeoObjectResolver(TContext& ctx, const TStringBuf searchText, const TMaybe<TGeoPosition>& searchPos,
        const TStringBuf geoType = "geo,biz", const TStringBuf sortBy = "",
        const TVector<TStringBuf>& businessFilters={}, const size_t resultIndex = 1, const bool includeExperimentalMeta = false);

    /** Create resolver object for reverse-resolving (from LL into geo object)
      * @param[in] ctx - context object (used to get data about user's location)
      * @param[in] ll - position to resolve
      * @param[in] kind - address type (possible values: empty string, "metro", "district", "locality")
      */
    TGeoObjectResolver(TContext& ctx, const TGeoPosition& ll, const TStringBuf zoom = TStringBuf(""));

    /** Create resolver object to get information about organization by its id
      * @param[in] ctx - context object (used to get data about user's location)
      * @param[in] orgId - organization id
      * @param[in] extendedInfo - request extended information (reviews, nearby stations, etc...)
      * @param[in] proto - use proto format for response
      */
    TGeoObjectResolver(TContext& ctx, TStringBuf orgId, bool extendedInfo = false, bool proto = false);

    /** Create resolver object to get information about multiple organizations by its id
      * @param[in] ctx - context object (used to get data about user's location)
      * @param[in] orgIds - organizations ids
      */
    TGeoObjectResolver(TContext& ctx, const TStringBuf searchText, const TVector<TStringBuf>& orgIds);

    /** Wait request and parse response
     * @param[out] firstGeo - first found result (toponym or POI as NSc::TValue)
     * @param[out] secondGeo - second found result (for suggest)
     * @param[out] resolvedWhere - resolved where part from search text (needed for POI search)
     * @return error object in case there were errors or empty maybe in case of success
     */
    TResultValue WaitAndParseResponse(NSc::TValue* firstGeo, NSc::TValue* secondGeo = nullptr, NSc::TValue* resolvedWhere = nullptr);

    /** Wait request and parse response
     * @param[out] results - found results (toponym or POI as NSc::TValue)
     * @param[out] docsCount - docs count to extract
     * @param[out] resolvedWhere - resolved where part from search text (needed for POI search)
     * @return error object in case there were errors or empty maybe in case of success
     */
    TResultValue WaitAndParseResponse(TVector<NSc::TValue>& results, size_t docsCount, NSc::TValue* resolvedWhere = nullptr);

    /** Wait request and parse response
     * @param[out] orgInfo - requested organization info
     * @return error object in case there were errors or empty maybe in case of success
     */
    TResultValue WaitAndParseOrganizationResponse(NSc::TValue* orgInfo);

    /** Wait request and parse response
     * @param[out] orgsInfo - requested organizations info
     * @return error object in case there were errors or empty maybe in case of success
     */
    TResultValue WaitAndParseMultiOrganizationResponse(NSc::TValue* allOrgInfo);

    /* If exists road with name `text` find it canonical name
     * @param[in] text - requested road toponym
     * @param[out] response - resolve canonical name
     * @return error object if not succeed
     */
    TResultValue WaitAndParseGeoCoderRoadResponse(const TString& text, TString* response);

    static TStringBuf GeoObjectType(const NSc::TValue& geoObject) {
        return NAlice::DetectGeoObjectType(geoObject);
    }

    /** If input geo object is country, then replace it with capital's data
     * @param[in] ctx - context object (needed to know lang, etc.)
     * @param[in|out] geoObject - NSc::TValue object, that will be changed
     */
    static void ReplaceCountryWithCapital(TContext& ctx, NSc::TValue* geoObject);

private:
    TResultValue DoParsePage(NHttpFetcher::THandle::TRef handle, const size_t resultIndexOnPage, const size_t docsCount,
                             TVector<NSc::TValue>& result, NSc::TValue* resolvedWhere = nullptr);

private:
    TContext& Ctx;
    TString SearchText;

    NHttpFetcher::IMultiRequest::TRef MultiRequest;
    NHttpFetcher::THandle::TRef GeoMetasearchRequest;
    NHttpFetcher::THandle::TRef NextPageRequest;
    TVector<NHttpFetcher::THandle::TRef> MultiGeoMetasearchRequest;

    size_t ResultIndexOnPage = 1;
};

}
