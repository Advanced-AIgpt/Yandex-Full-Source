#pragma once

#include <alice/hollywood/library/gif_card/proto/gif.pb.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>
#include <alice/hollywood/library/scenarios/general_conversation/rankers/memory_ranker.h>

#include <alice/hollywood/library/scenarios/suggesters/movie_akinator/movie_base.h>

#include <alice/hollywood/library/resources/resources.h>

#include <alice/begemot/lib/microintents_index/microintents_index.h>

#include <alice/boltalka/emoji/applier/emoji_classifier.h>

#include <alice/nlu/libs/binary_classifier/boltalka_dssm_embedder.h>

namespace NAlice::NHollywood::NGeneralConversation {

constexpr TStringBuf NLU_SEARCH_MODEL_NAME = "nlu_search_model";
constexpr TStringBuf NLU_SEARCH_CONTEXT_MODEL_NAME = "nlu_search_context_model";
constexpr TStringBuf LSTM256_MODEL_NAME = "lstm256";

using TSentimentConditionedQuestions = THashMap<TEntityDiscussion::EDiscussionSentiment, TVector<TString>>;

struct TMicrointent {
    struct TMusicInfo {
        TString Query;
    };

    double Threshold;
    double ModalThreshold;
    TVector<TString> Suggests;
    TVector<TVector<TString>> LedImages;
    bool IsGcFallback;
    bool ShouldNotListen;
    TString Emotion;
    bool IsBirthday;
    TVector<TString> ActivationPhrases;
    TVector<TString> EllipsisIntents;
    THashMap<TString, TMusicInfo> MusicActions;
    bool IsAllowed;
};

class TGeneralConversationResources final : public IResourceContainer {
public:
    void LoadFromPath(const TFsPath& dirPath) override;

    const NAlice::TBoltalkaDssmEmbedder* GetEmbedder(TStringBuf name) const;
    const TMemoryRanker* GetRanker(TStringBuf name) const;
    const THashMap<TString, TMicrointent>& GetMicrointents() const;
    const THashMap<TString, TVector<TGif>>& GetEmotionalGifs() const;
    const THashMap<TString, TVector<TString>>& GetEmotionalLedImages() const;
    const ::NNlg::TEmojiClassifier* GetEmojiClassifier() const;
    const THashMap<TString, ui32>& GetOntoToKpIdMapping() const;
    const TEntity* GetEntity(const TString& id) const;
    const TVector<TGif>& GetUnclassifiedGifs() const;
    const TVector<TString>* GetEntityQuestions(const TString& id, TEntityDiscussion::EDiscussionSentiment sentiment) const;
    const THashMap<TString, TVector<TString>>& GetCrossPromoFacts(bool useFilteredDict = false) const;
    const TVector<TMovie>& GetMoviesToDiscuss() const;
    const TVector<TMovie>& GetMoviesToDiscussByType(const TString& movieType) const;
    const TClusteredMovies& GetClusteredMovies() const;
    const NBg::TMicrointentsIndex* GetMicrointentsClassifier() const;

private:
    THashMap<TString, NAlice::TBoltalkaDssmEmbedder> Embedders_;
    THashMap<TString, TMemoryRanker> MemoryRankers_;
    THashMap<TString, TMicrointent> Microintents_;
    THashMap<TString, TVector<TGif>> EmotionalGifs_;
    THashMap<TString, TVector<TString>> EmotionalLedImages_;
    THolder<::NNlg::TEmojiClassifier> EmojiClassifier_;
    THashMap<TString, ui32> OntoToKpIdMapping_;
    THashMap<TString, TEntity> KnownEntities_;
    TVector<TGif> UnclassifiedGifs_;
    THashMap<TString, TSentimentConditionedQuestions> EntityToQuestions_;
    THashMap<TString, TVector<TString>> CrossPromoFacts_;
    THashMap<TString, TVector<TString>> CrossPromoFactsFiltered_;
    TVector<TMovie> MoviesToDiscuss_;
    THashMap<TString, TVector<TMovie>> MoviesToDiscussByType_;
    TClusteredMovies ClusteredMovies_;
    THolder<NBg::TMicrointentsIndex> MicrointentsClassifier_;
};

} // namespace NAlice::NHollywood::NGeneralConversation
