#pragma once

#include <alice/hollywood/library/scenarios/video/video_utils.h>

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NVideo {

inline static constexpr TStringBuf OPEN_ITEM = "open_item";

class TVideoOpenItemScene : public TScene<TOpenItemSceneArgs> {
public:
    TVideoOpenItemScene(const TScenario* owner)
        : TScene(owner, OPEN_ITEM)
    {
        RegisterRenderer(&TVideoOpenItemScene::OpenCentaurVideo);
        RegisterRenderer(&TVideoOpenItemScene::OpenCollection);
        RegisterRenderer(&TVideoOpenItemScene::OpenOttVideo);
        RegisterRenderer(&TVideoOpenItemScene::OpenPerson);
        RegisterRenderer(&TVideoOpenItemScene::OpenVideo);
    }

    TRetMain Main(
        const TOpenItemSceneArgs&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const;

    TRetResponse OpenOttVideo(
        const TOttVideoItem&,
        TRender&) const;
    TRetResponse OpenVideo(
        const TVideoItem&,
        TRender&) const;
    TRetResponse OpenCentaurVideo(
        const NRenderer::TDivRenderData&,
        TRender&) const;

    static TRetResponse OpenPerson(
        const TPersonItem&,
        TRender&);
    static TRetResponse OpenCollection(
        const TCollectionItem&,
        TRender&);

    // In future
    static TRetResponse OpenApplication(
        const NTv::TRecentApplicationsItem&,
        TRender&) = delete;
    static TRetResponse OpenMusic(
        const NTv::TMusicItem&,
        TRender&) = delete;
};

} // namespace NAlice::NHollywoodFw::NVideo
