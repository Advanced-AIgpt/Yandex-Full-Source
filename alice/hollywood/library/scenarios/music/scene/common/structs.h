#pragma once

#include <alice/hollywood/library/framework/core/request.h>
#include <alice/hollywood/library/framework/core/run_features.h>
#include <alice/hollywood/library/framework/core/source.h>
#include <alice/hollywood/library/framework/core/storage.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/repeated_skip.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

struct TCommandInfo;
struct TScenarioRequestData;
struct TScenarioStateData;
struct TCommonRenderData;

struct TCommandInfo {
    struct TActionInfo {
        TStringBuf Id;
        TStringBuf Name;
        TStringBuf HumanReadable;
    };

    TStringBuf ProductScenarioName;
    TStringBuf Intent;
    TStringBuf NlgTemplate;
    TMaybe<TActionInfo> ActionInfo;
};

struct TScenarioRequestData {
    const TRequest& Request;
    TStorage& Storage;
    const TSource* Source = nullptr; // is null in XxxSetup methods

    template<typename TProto> TProto GetProto() const {
        return Request.Client().TryGetMessage<TProto>().GetOrElse(TProto{});
    }

    void FillAnalyticsInfo(const TCommandInfo& commandInfo, const TScenarioStateData& state);
    void FillAnalyticsInfoFirstTrackObject(const NHollywood::NMusic::TQueueItem& curItem);
};

struct TScenarioStateData {
    NHollywood::NMusic::TScenarioState ScenarioState;

    // the flag indicating whether `ScenarioState` unpacked successfully
    const bool HaveState;

    // the wrapper for accessing ang changing ScenarioState's Queue.
    // WARNING: don't forget to call `Storage.SetScenarioState(ScenarioState)`
    //          after changing the queue!
    NHollywood::NMusic::TMusicQueueWrapper MusicQueue;

    // the wrapper for accessing ang changing ScenarioState's RepeatedSkipState.
    NHollywood::NMusic::TRepeatedSkip RepeatedSkip;

public:
    TScenarioStateData(TScenarioRequestData& requestData);
    ~TScenarioStateData();

    void DontSaveChangedState();

private:
    TStorage& Storage_;
    const TString Hash_;
    bool ShouldSaveChangedState_ = true;
};

struct TCommonRenderData {
    TRunFeatures RunFeatures;
    TMusicScenarioRenderArgsCommon RenderArgs;
    TMaybe<NRenderer::TDivRenderData> DivRenderData;

    void FillRunFeatures(const TScenarioRequestData& requestData);
};

} // namespace NAlice::NHollywoodFw::NMusic
