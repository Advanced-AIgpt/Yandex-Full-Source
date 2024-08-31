#pragma once

#include <alice/hollywood/library/framework/framework.h>

#include <alice/hollywood/library/scenarios/video/proto/video_scene_args.pb.h>
#include <alice/protos/data/video/video_scenes.pb.h>


namespace NAlice::NHollywoodFw::NVideo {

inline constexpr TStringBuf GRPC_OPEN_SEARCH = "grpc_open_search";
inline constexpr TStringBuf OPEN_SEARCH = "open_search";


class TVideoOpenSearchScene : public TScene<TVideoSearchResultArgs> {
public:
    TVideoOpenSearchScene(const TScenario *owner) : TScene(owner, OPEN_SEARCH)
    {
        RegisterRenderer(&TVideoOpenSearchScene::RenderSearch);
        RegisterRenderer(&TVideoOpenSearchScene::RenderSearchShowView);
    }

    TRetMain Main(
        const TVideoSearchResultArgs&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const;

    TRetResponse RenderSearch(
        const TVideoSearchResultArgs&,
        TRender&) const;

    static TRetResponse RenderSearchShowView(
        const NRenderer::TDivRenderData&,
        TRender&);
};


class TVideoGRPCOpenSearchScene : public TScene<TGetTvSearchResultSemanticFrame> {
public:
    TVideoGRPCOpenSearchScene(const TScenario *owner) : TScene(owner, GRPC_OPEN_SEARCH) {
        RegisterRenderer(&TVideoGRPCOpenSearchScene::RenderSearch);
    }

    TRetMain Main(
        const TGetTvSearchResultSemanticFrame&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const;

    TRetResponse RenderSearch(
        const TVideoSearchResultArgs&,
        TRender&) const;
};

inline TVideoSearchCallArgs MakeSearchCallArgs(const TString& searchText,
                                                TRTLogger& logger,
                                                const TString& restrictionMode = "",
                                                const TString& restrictionAge = "",
                                                const TString& entref = "") {
    TVideoSearchCallArgs searchArgs;
    searchArgs.SetSearchText(searchText);
    LOG_DEBUG(logger) << "Going to search \"" << searchText
                        << (!entref.Empty() ? "\nEntref: \"" + entref + '"' : "")
                        << (!restrictionMode.Empty() ? "\nRestrictions: " + restrictionMode : "")
                        << (restrictionMode == "kids" && !restrictionAge.Empty() ? "\nRestriction age for kids: " + restrictionAge : "")
                        << '"';
    if (restrictionMode == "kids") {
        LOG_DEBUG(logger) << " - age " << restrictionAge;
    }
    if (!entref.Empty()) {
        searchArgs.SetSearchEntref(entref);
    }
    if (!restrictionMode.Empty()) {
        searchArgs.SetRestrictionMode(restrictionMode);

        if (!restrictionAge.Empty()) {
            searchArgs.SetRestrictionAge(restrictionAge);
        }
    }
    return searchArgs;
}

} // namespace NAlice::NHollywoodFw::NVideo
