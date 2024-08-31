#pragma once

#include "callback_directive_model.h"
#include "client_directive_model.h"

#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>

#include <utility>

namespace NAlice::NMegamind {

enum class EStreamFormat {
    Unknown /* "Unknown" */,
    MP3 /* "MP3" */,
    HLS /* "HLS" */,
};

enum class EStreamType {
    Track /* "Track" */,
    Shot /* "Shot" */,
};

enum class EBackgroundMode {
    Ducking /* "Ducking" */,
    Pause /* "Pause" */,
};

enum class EScreenType {
    Default /* "Default" */,
    Music /* "Music" */,
    Radio /* "Radio" */,
};

enum class EContentType {
    Track /* "Track" */,
    Album /* "Album" */,
    Artist /* "Artist" */,
    Playlist /* "Playlist" */,
    Radio /* "Radio" */,
    Generative /* "Generative" */,
    FmRadio /* "FmRadio" */,
};

enum class EInnerGlagolMetadata {
    Stub /* Stub */,
    MusicMetadata /* "music_metadata" */,
};

enum class ERepeatMode {
    Unknown,
    None,
    One,
    All
};

class IInnerGlagolMetadataModel : public IModel, public TThrRefBase {
public:
    [[nodiscard]] virtual EInnerGlagolMetadata GetInnerGlagolMetadataType() const = 0;
};

struct TPrevNextTrackInfo {
    TPrevNextTrackInfo(const TString& id, EStreamType streamType)
        : Id(id)
        , StreamType(streamType) {
    }

    TString Id;
    EStreamType StreamType;
};

class TMusicMetadataModel final : public IInnerGlagolMetadataModel {
public:
    TMusicMetadataModel(const TString& id, EContentType type, const TString& description,
                        const TPrevNextTrackInfo& prevTrackInfo, const TPrevNextTrackInfo& nextTrackInfo,
                        const TMaybe<bool> shuffled, const TMaybe<ERepeatMode> repeatMode)
        : Id(id)
        , Type(type)
        , Description(description)
        , PrevTrackInfo(prevTrackInfo)
        , NextTrackInfo(nextTrackInfo)
        , Shuffled(shuffled)
        , RepeatMode(repeatMode) {
    }

    void Accept(IModelSerializer& serializer) const final {
        serializer.Visit(*this);
    }

    [[nodiscard]] virtual EInnerGlagolMetadata GetInnerGlagolMetadataType() const final {
        return EInnerGlagolMetadata::MusicMetadata;
    }

    [[nodiscard]] const TString& GetId() const {
        return Id;
    }

    [[nodiscard]] EContentType GetType() const {
        return Type;
    }

    [[nodiscard]] const TString& GetDescription() const {
        return Description;
    }

    [[nodiscard]] const TPrevNextTrackInfo& GetPrevTrackInfo() const {
        return PrevTrackInfo;
    }

    [[nodiscard]] const TPrevNextTrackInfo& GetNextTrackInfo() const {
        return NextTrackInfo;
    }

    [[nodiscard]] TMaybe<bool> GetShuffled() const {
        return Shuffled;
    }

    [[nodiscard]] TMaybe<ERepeatMode> GetRepeatMode() const {
        return RepeatMode;
    }

private:
    TString Id;
    EContentType Type;
    TString Description;
    TPrevNextTrackInfo PrevTrackInfo;
    TPrevNextTrackInfo NextTrackInfo;
    TMaybe<bool> Shuffled;
    TMaybe<ERepeatMode> RepeatMode;
};

class TStubInnerGlagolMetadataModel final: public IInnerGlagolMetadataModel {
public:
    void Accept(IModelSerializer& serializer) const final {
        serializer.Visit(*this);
    }

    [[nodiscard]] virtual EInnerGlagolMetadata GetInnerGlagolMetadataType() const final {
        return EInnerGlagolMetadata::Stub;
    }
};

class TGlagolMetadataModel final : public IModel {
public:
    TGlagolMetadataModel(TIntrusivePtr<IInnerGlagolMetadataModel> innerGlagolMetaData)
        : InnerGlagolMetadata(std::move(innerGlagolMetaData))
    {
        Y_ASSERT(InnerGlagolMetadata);
    }

    void Accept(IModelSerializer& serializer) const final {
        serializer.Visit(*this);
    }

    [[nodiscard]] const IInnerGlagolMetadataModel& GetInnerGlagolMetadata() const {
        return *InnerGlagolMetadata;
    }

private:
    TIntrusivePtr<IInnerGlagolMetadataModel> InnerGlagolMetadata;
};

class TAudioPlayDirectiveStreamNormalizationModel final : public IModel {
public:
    TAudioPlayDirectiveStreamNormalizationModel(double integratedLoudness, double truePeak)
        : IntegratedLoudness(integratedLoudness)
        , TruePeak(truePeak) {
    }

    void Accept(IModelSerializer& serializer) const final {
        serializer.Visit(*this);
    }

    [[nodiscard]] double GetIntegratedLoudness() const {
        return IntegratedLoudness;
    }

    [[nodiscard]] double GetTruePeak() const {
        return TruePeak;
    }

private:
    double IntegratedLoudness;
    double TruePeak;
};

class TAudioPlayDirectiveStreamModel final : public virtual IModel {
public:
    TAudioPlayDirectiveStreamModel(const TString& id, const TString& url, int offsetMs,
                                   EStreamFormat streamFormat, EStreamType streamType,
                                   const TMaybe<TAudioPlayDirectiveStreamNormalizationModel>& normalization)
        : Id(id)
        , Url(url)
        , OffsetMs(offsetMs)
        , StreamFormat(streamFormat)
        , StreamType(streamType)
        , Normalization(normalization) {
    }

    void Accept(IModelSerializer& serializer) const final {
        serializer.Visit(*this);
    }

    [[nodiscard]] const TString& GetId() const {
        return Id;
    }

    [[nodiscard]] const TString& GetUrl() const {
        return Url;
    }

    [[nodiscard]] int GetOffsetMs() const {
        return OffsetMs;
    }

    [[nodiscard]] EStreamFormat GetStreamFormat() const {
        return StreamFormat;
    }

    [[nodiscard]] EStreamType GetStreamType() const {
        return StreamType;
    }

    [[nodiscard]] const TMaybe<TAudioPlayDirectiveStreamNormalizationModel>& GetNormalization() const {
        return Normalization;
    }

private:
    TString Id;
    TString Url;
    int OffsetMs;
    EStreamFormat StreamFormat;
    EStreamType StreamType;
    TMaybe<TAudioPlayDirectiveStreamNormalizationModel> Normalization;
};

class TAudioPlayDirectiveMetadataModel final : public virtual IModel {
public:
    TAudioPlayDirectiveMetadataModel(const TString& title,
                                     const TString& subTitle,
                                     const TString& artImageUrl,
                                     TGlagolMetadataModel& glagolMetadata,
                                     bool hideProgressBar)
        : Title(title)
        , SubTitle(subTitle)
        , ArtImageUrl(artImageUrl)
        , GlagolMetadata(glagolMetadata)
        , HideProgressBar(hideProgressBar) {
    }

    void Accept(IModelSerializer& serializer) const final {
        serializer.Visit(*this);
    }

    [[nodiscard]] const TString& GetTitle() const {
        return Title;
    }

    [[nodiscard]] const TString& GetSubTitle() const {
        return SubTitle;
    }

    [[nodiscard]] const TString& GetArtImageUrl() const {
        return ArtImageUrl;
    }

    [[nodiscard]] const TGlagolMetadataModel& GetGlagolMetadata() const {
        return GlagolMetadata;
    }

    [[nodiscard]] bool GetHideProgressBar() const {
        return HideProgressBar;
    }

private:
    TString Title;
    TString SubTitle;
    TString ArtImageUrl;
    TGlagolMetadataModel GlagolMetadata;
    bool HideProgressBar;
};

class TAudioPlayDirectiveCallbacksModel final : public virtual IModel {
public:
    TAudioPlayDirectiveCallbacksModel(TIntrusivePtr<TCallbackDirectiveModel> onPlayStartedCallback,
                                      TIntrusivePtr<TCallbackDirectiveModel> onPlayStoppedCallback,
                                      TIntrusivePtr<TCallbackDirectiveModel> onPlayFinishedCallback,
                                      TIntrusivePtr<TCallbackDirectiveModel> onFailedCallback)
        : OnPlayStartedCallback(std::move(onPlayStartedCallback))
        , OnPlayStoppedCallback(std::move(onPlayStoppedCallback))
        , OnPlayFinishedCallback(std::move(onPlayFinishedCallback))
        , OnFailedCallback(std::move(onFailedCallback)) {
    }

    void Accept(IModelSerializer& serializer) const final {
        serializer.Visit(*this);
    }

    [[nodiscard]] const TIntrusivePtr<TCallbackDirectiveModel>& GetOnPlayStartedCallback() const {
        return OnPlayStartedCallback;
    }

    [[nodiscard]] const TIntrusivePtr<TCallbackDirectiveModel>& GetOnPlayStoppedCallback() const {
        return OnPlayStoppedCallback;
    }

    [[nodiscard]] const TIntrusivePtr<TCallbackDirectiveModel>& GetOnPlayFinishedCallback() const {
        return OnPlayFinishedCallback;
    }

    [[nodiscard]] const TIntrusivePtr<TCallbackDirectiveModel>& GetOnFailedCallback() const {
        return OnFailedCallback;
    }

private:
    TIntrusivePtr<TCallbackDirectiveModel> OnPlayStartedCallback;
    TIntrusivePtr<TCallbackDirectiveModel> OnPlayStoppedCallback;
    TIntrusivePtr<TCallbackDirectiveModel> OnPlayFinishedCallback;
    TIntrusivePtr<TCallbackDirectiveModel> OnFailedCallback;
};

class TAudioPlayDirectiveModel final : public TClientDirectiveModel {
public:
    TAudioPlayDirectiveModel(const TString& analyticsType, TAudioPlayDirectiveStreamModel& stream,
                             TAudioPlayDirectiveMetadataModel& audioPlayMetadata,
                             TAudioPlayDirectiveCallbacksModel& callbacks,
                             THashMap<TString, TString>& scenarioMeta,
                             EBackgroundMode backgroundMode,
                             const TString& providerName,
                             EScreenType screenType,
                             const bool setPause,
                             TMaybe<TString> multiroomToken)
        : TClientDirectiveModel("audio_play", analyticsType)
        , Stream(std::move(stream))
        , AudioPlayMetadata(std::move(audioPlayMetadata))
        , Callbacks(std::move(callbacks))
        , ScenarioMeta(std::move(scenarioMeta))
        , BackgroundMode(backgroundMode)
        , ProviderName(providerName)
        , ScreenType(screenType)
        , SetPause(setPause)
        , MultiroomToken(std::move(multiroomToken)) {
    }

    void Accept(IModelSerializer& serializer) const final {
        serializer.Visit(*this);
    }

    [[nodiscard]] const TAudioPlayDirectiveStreamModel& GetStream() const {
        return Stream;
    }

    [[nodiscard]] const TAudioPlayDirectiveMetadataModel& GetAudioPlayMetadata() const {
        return AudioPlayMetadata;
    }

    [[nodiscard]] const TAudioPlayDirectiveCallbacksModel& GetCallbacks() const {
        return Callbacks;
    }

    [[nodiscard]] const THashMap <TString, TString>& GetScenarioMeta() const {
        return ScenarioMeta;
    }

    [[nodiscard]] EBackgroundMode GetBackgroundMode() const {
        return BackgroundMode;
    }

    [[nodiscard]] const TString& GetProviderName() const {
        return ProviderName;
    }

    [[nodiscard]] EScreenType GetScreenType() const {
        return ScreenType;
    }

    [[nodiscard]] bool GetSetPause() const {
        return SetPause;
    }

    [[nodiscard]] const TMaybe<TString>& GetMultiroomToken() const {
        return MultiroomToken;
    }

private:
    TAudioPlayDirectiveStreamModel Stream;
    TAudioPlayDirectiveMetadataModel AudioPlayMetadata;
    TAudioPlayDirectiveCallbacksModel Callbacks;
    THashMap<TString, TString> ScenarioMeta;
    EBackgroundMode BackgroundMode;
    TString ProviderName;
    EScreenType ScreenType;
    bool SetPause;
    TMaybe<TString> MultiroomToken;
};

} // namespace NAlice::NMegamind
