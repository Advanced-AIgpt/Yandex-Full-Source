#include "search_metrics.h"

#include <alice/library/search_result_parser/search_result_parser.h>
#include <alice/library/search_result_parser/video/parsers.h>
#include <alice/library/proto/proto_adapter.h>

#include <format>

namespace NAlice::NHollywoodFw::NVideo::NSearchMetrics {
    namespace Monitoring::Web {
        NMonitoring::TLabels CommonLabels() {
            static const NMonitoring::TLabels labels{
                {"scenario_name", "video"},
                {"name", "web_search_result"},
            };
            return labels;
        }

        void IncCounterAndLog(NAlice::NMetrics::ISensors& sensors, const TMap<TString, TString>& labelsMap, TRTLogger& logger, std::string_view text) {
            NMonitoring::TLabels labels{CommonLabels()};
            for (const auto& [k, v] : labelsMap) {
                labels.Add(k, v);
            }
            sensors.IncRate(labels);

            TString msg{std::format("VWSM: {}", text)};
            LOG_INFO(logger) << msg;
        }

        void TrackNoDatasource(NAlice::NMetrics::ISensors& sensors, TRTLogger& logger) {
            static TMap<TString, TString> noDSLabels = {{"subname", "no_datasource"}};
            IncCounterAndLog(sensors, noDSLabels, logger, "no_datasource");
        }

        void TrackNoSnippet(NAlice::NMetrics::ISensors& sensors, TRTLogger& logger) {
            static TMap<TString, TString> noSnippetLabels = {{"subname", "no_snippet"}};
            IncCounterAndLog(sensors, noSnippetLabels, logger, "no_snippet");
        }

        void TrackWebSnippet(NAlice::NMetrics::ISensors& sensors, TRTLogger& logger, const GStruct& snippet) {
            TProtoStructParser parser;
            auto testKey = [&parser, &snippet](const TString key) {
                TProtoStructParser::EResult t = parser.TestKey(snippet, key);
                return t != TProtoStructParser::EResult::Absent && t != TProtoStructParser::EResult::Null;
            };
            auto trace = [&sensors, &logger](const TMap<TString, TString>& labelsMap, std::string_view text) {
                IncCounterAndLog(sensors, labelsMap, logger, text);
            };
            if (testKey("data.parent_collection.object")) {
                static TMap<TString, TString> parentLabels = {{"subname", "has_parent_collection"}};
                trace(parentLabels, "has_parent_collection");
            }
            if (testKey("data.related_object")) {
                static TMap<TString, TString> relatedLabels = {{"subname", "has_related_object"}};
                trace(relatedLabels, "has_related_object");
            }
            if (testKey("data.base_info")) {
                static TMap<TString, TString> baseInfoLabels = {{"subname", "has_base_info"}};
                trace(baseInfoLabels, "has_base_info");
            };
            if (testKey("data.base_info.type")) {
                TString type = parser.GetValueString(snippet, "data.base_info.type", "Default");
                // series and shows has type "Film" too
                if (type == "Film") {
                    static TMap<TString, TString> filmBaseInfoLabels = {{"subname", "film_base_info"}};
                    trace(filmBaseInfoLabels, "film_base_info");
                } else {
                    return;
                }
            } else {
                return;
            }
            if (testKey("data.base_info.id")) {
                static TMap<TString, TString> ontoIdLabels = {{"subname", "has_onto_id"}};
                trace(ontoIdLabels, "has_onto_id");
            }
            // 'no license' vs 'trailer license' vs 'other license'
            if (testKey("data.base_info.legal.vh_licenses")) {
                if (testKey("data.base_info.legal.vh_licenses.content_type")) {
                    TString type = parser.GetValueString(snippet, "data.base_info.legal.vh_licenses.content_type", "Default");
                    if (type == "TRAILER" || type == "KP_TRAILER") {
                        static TMap<TString, TString> trailerBaseInfoLabels = {{"subname", "vh_license"}, {"license", "trailer"}};
                        trace(trailerBaseInfoLabels, "vh_license - trailer");
                    } else {
                        static TMap<TString, TString> otherBaseInfoLabels = {{"subname", "vh_license"}, {"license", "other"}};
                        trace(otherBaseInfoLabels, "vh_license - other");
                    }
                }
            } else {
                static TMap<TString, TString> noLicenseLabels = {{"subname", "vh_license"}, {"license", "no_license"}};
                trace(noLicenseLabels, "vh_license - no_license");
            }
        }
    }

    void TrackWebSearchEntitySnippet(const TRunRequest& request) {
        NMetrics::ISensors& sensors = request.System().GetSensors();
        TRTLogger& logger = request.Debug().Logger();

        const auto* webDS = request.GetDataSource(NAlice::EDataSourceType::WEB_SEARCH_DOCS);
        if (webDS != nullptr) {
            NAlice::TSearchResultParser searchResultParser(TRTLogger::NullLogger());
            searchResultParser.AttachDataSource(webDS);

            auto esSnippet = searchResultParser.FindSnippetByType({TSearchResultParser::EUseDatasource::Docs}, "entity_search");
            if (esSnippet.Defined()) {
                Monitoring::Web::TrackWebSnippet(sensors, logger, *esSnippet);
            } else {
                Monitoring::Web::TrackNoSnippet(sensors, logger);
            }
        } else {
            Monitoring::Web::TrackNoDatasource(sensors, logger);
        }
    }

    void TrackWhichSearchResultWasUsed(NMetrics::ISensors& sensors, enum ESearchResultSource source) {
        auto toString = [](const enum ESearchResultSource source) {
            switch (source) {
                case WebSearchBaseInfo:
                    return TString("web_base_info");
                case WebSearchAll:
                    return TString("web_search_all");
                case VideoSearchBaseInfo:
                    return TString("vs_base_info");
                case VideoSearchAll:
                    return TString("vs_all");
                default:
                    return TString("other");
            }
        };
        static const NMonitoring::TLabels common{
            {"scenario_name", "video"},
            {"name", "search_result_source"},
        };
        NMonitoring::TLabels labels(common);
        labels.Add("source", toString(source));
        sensors.IncRate(labels);
    }

    namespace Monitoring::VideoSearch {
        NMonitoring::TLabels CommonLabels() {
            static const NMonitoring::TLabels labels{
                {"scenario_name", "video"},
                {"name", "video_search_result"},
            };
            return labels;
        }

        void IncCounter(NAlice::NMetrics::ISensors& sensors, const TMap<TString, TString>& labelsMap) {
            NMonitoring::TLabels labels{CommonLabels()};
            for (const auto& [k, v] : labelsMap) {
                labels.Add(k, v);
            }
            sensors.IncRate(labels);
        }
    }

    void TrackVideoSearchResponded(NMetrics::ISensors& sensors, bool success) {
        TMap<TString, TString> labels = {{"subname", "response"}};
        if (success) {
            labels.emplace("success", "true");
        } else {
            labels.emplace("success", "false");
        }
        Monitoring::VideoSearch::IncCounter(sensors, labels);
    }

    void TrackVideoSearchResult(NMetrics::ISensors& sensors, const TMaybe<TTvSearchResultData>& result) {
        if (!result.Defined()) {
            Monitoring::VideoSearch::IncCounter(sensors, {{"subname", "search_result"}, {"search_data_is_empty", "true"}});
            return;
        }
        Monitoring::VideoSearch::IncCounter(sensors, {{"subname", "search_result"}, {"search_data_is_empty", "false"}});
        for (const auto& gallery : result->GetGalleries()) {
            if (gallery.HasBasicCarousel()) {
                Monitoring::VideoSearch::IncCounter(sensors, {{"subname", "search_result"}, {"has_gallery", gallery.GetBasicCarousel().GetTitle()}});   
            } else {
                Monitoring::VideoSearch::IncCounter(sensors, {{"subname", "search_result"}, {"has_gallery", "EMPTY!"}});   
            }
        }
    }

    void TrackVideoSearchBaseInfo(NMetrics::ISensors& sensors, const TMaybe<NTv::TCarouselItemWrapper> baseInfo) {
        TMap<TString, TString> labels = {{"subname", "search_result"}};
        // film vs other vs not_exist
        if (baseInfo.Defined()) {
            if (baseInfo->has_videoitem()) {
                labels.emplace("base_info", "film");
            } else {
                labels.emplace("base_info", "other");
            }
        } else {
            labels.emplace("base_info", "not_exists");
        }
        Monitoring::VideoSearch::IncCounter(sensors, labels);       
    }
}
