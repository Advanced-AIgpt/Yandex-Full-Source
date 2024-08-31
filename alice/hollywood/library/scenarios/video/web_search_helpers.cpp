#include "web_search_helpers.h"

#include <alice/library/video_common/defs.h>

#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/scenarios/web_search_source.pb.h>

#include <alice/protos/data/search_result/tv_search_result.pb.h>

#include <alice/library/search_result_parser/search_result_parser.h>
#include <alice/library/search_result_parser/video/parsers.h>
#include <alice/library/proto/proto_adapter.h>

#include <util/stream/file.h>

namespace NAlice::NHollywoodFw::NVideo::WebSearchHelpers {

    static TMaybe<google::protobuf::Struct> FindEntitySearchSnippet(const TRunRequest &request) {
        NAlice::TSearchResultParser searchResultParser(TRTLogger::NullLogger());
        const auto* web = request.GetDataSource(NAlice::EDataSourceType::WEB_SEARCH_DOCS);
        if (web != nullptr) {
            searchResultParser.AttachDataSource(web);
        }

        const auto* webRight = request.GetDataSource(NAlice::EDataSourceType::WEB_SEARCH_DOCS_RIGHT);
        if (webRight != nullptr) {
            searchResultParser.AttachDataSource(webRight);
        }

        return std::move(searchResultParser.FindDocSnippetByType("entity_search"));
    }

    static void DumpToFile(const google::protobuf::Message& message, const TString& docName) {
        TString msgJSON = JsonStringFromProto(message);
        TFileOutput output(docName);
        output << msgJSON;
    }

    bool HasUsefulSnippet(const TRunRequest& request, TRTLogger& logger) {
        const TTvSearchResultData searchData = ParseSearchSnippetFromWeb(request, logger);
        return searchData.GalleriesSize();
    }

    TTvSearchResultData ParseSearchSnippetFromWeb(const TRunRequest& request, TRTLogger& logger) {
        const auto esSnippet = FindEntitySearchSnippet(request);

        if (!esSnippet.Defined()) {
            return {};
        }
        if (request.Flags().IsExperimentEnabled(NVideoCommon::FLAG_VIDEO_DUMP_WEB_SEARCH)) {
            DumpToFile(esSnippet.GetRef(), "input.json");
        }

        TProtoAdapter adapter(esSnippet.GetRef());

        bool useHalfPiratesFromBaseInfo = request.Flags().IsExperimentEnabled(NVideoCommon::FLAG_VIDEO_HALFPIRATE_FROM_BASEINFO);
        return NHollywood::NVideo::SearchResultParser::ParseProtoResponse(adapter, logger, useHalfPiratesFromBaseInfo);
    }

    TMaybe<NTv::TCarouselItemWrapper> ParseBaseInfoFromWeb(const TRunRequest& request, TRTLogger& logger) {
        const auto esSnippet = FindEntitySearchSnippet(request);

        if (!esSnippet.Defined()) {
            return Nothing();
        }

        TProtoAdapter adapter(esSnippet.GetRef());

        return NHollywood::NVideo::SearchResultParser::ParseBaseInfo(adapter["data"], logger);
    }
}

