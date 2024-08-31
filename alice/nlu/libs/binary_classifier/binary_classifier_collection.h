#pragma once

#include <alice/nlu/libs/binary_classifier/beggins_binary_classifier.h>
#include <alice/nlu/libs/binary_classifier/boltalka_dssm_embedder.h>
#include <alice/nlu/libs/binary_classifier/dssm_based_binary_classifier.h>
#include <alice/nlu/libs/binary_classifier/lstm_based_binary_classifier.h>
#include <alice/nlu/libs/binary_classifier/mixed_input.h>
#include <alice/nlu/libs/binary_classifier/mixed_classifier.h>

#include <alice/begemot/lib/fresh_options/proto/fresh_options.pb.h>
#include <alice/nlu/granet/lib/sample/entity.h>

#include <search/begemot/core/filesystem.h>
#include <search/begemot/core/parallel_executor.h>

#include <util/folder/path.h>
#include <util/generic/hash_set.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {

// ~~~~ EBinaryClassifierType ~~~~

enum EBinaryClassifierType {
    BCT_LSTM_CLASSIFIER,
    BCT_DSSM_CLASSIFIER,
    BCT_BEGGINS_CLASSIFIER,
    BCT_ZELIBOBA_CLASSIFIER,
    BCT_MIXED_CLASSIFIER,
};

// ~~~~ TBinaryClassifierVariant ~~~~

class TBinaryClassifierVariant : TMoveOnly {
public:
    explicit TBinaryClassifierVariant(const TString& name);

    const TString& GetName() const;
    const TFsPath& GetModelPath() const;

    void Load(const NBg::TFileSystem& fs, const TFsPath& path, EBinaryClassifierType type,
        const TTokenEmbedder& tokenEmbedder);

    void LoadLstmClassifier(const NBg::TFileSystem& fs, const TFsPath& path, const TTokenEmbedder& tokenEmbedder);
    void LoadDssmClassifier(const NBg::TFileSystem& fs, const TFsPath& path);
    void LoadBegginsClassifier(const NBg::TFileSystem& fs, const TFsPath& path);
    void LoadZelibobaClassifier(const NBg::TFileSystem& fs, const TFsPath& path);
    void LoadMixedClassifier(const NBg::TFileSystem& fs, const TFsPath& path);

    bool IsInitialized() const;

    bool IsLstmClassifier() const;
    bool IsDssmClassifier() const;
    bool IsBegginsClassifier() const;
    bool IsZelibobaClassifier() const;
    bool IsMixedClassifier() const;

    bool NeedSentenceEmebedding(TStringBuf embeddingName) const;
    bool NeedSentenceFeatures() const;

    float Predict(const TMixedBinaryClassifierInput& input) const;

private:
    TString Name;
    TFsPath ModelPath;
    TLstmBasedBinaryClassifierPtr LstmClassifier;
    TDssmBasedBinaryClassifierPtr DssmClassifier;
    TBegginsBinaryClassifierPtr BegginsClassifier;
    TBegginsBinaryClassifierPtr ZelibobaClassifier;
    TMixedBinaryClassifierPtr MixedClassifier;
};

using TBinaryClassifierVariantPtr = THolder<TBinaryClassifierVariant>;

// ~~~~ EBinaryClassifierCollectionType ~~~~

enum EBinaryClassifierCollectionType {
    BCCT_STATIC     /* "static" */,
    BCCT_FRESH      /* "fresh" */,
    BCCT_DEV        /* "dev" */,
};

// ~~~~ TBinaryClassifierCollection ~~~~

class TBinaryClassifierCollection {
public:
    void Load(const NBg::TFileSystem& fs, const TFsPath& dir, const TTokenEmbedder& tokenEmbedder);

    const TVector<TString>& GetInitializationErrors() const;

    TVector<TString> EnumerateClassifiers() const;

    void Select(
        const TVector<TString>& enabledLstmClassifiers,
        const TVector<TString>& enabledDevClassifiers,
        bool enableBeggins,
        bool enableZeliboba,
        const TFreshOptions& freshOptions,
        EBinaryClassifierCollectionType collectionType,
        TVector<const TBinaryClassifierVariant*>* selected) const;

private:
    void LoadClassifiersOfType(
        const NBg::TFileSystem& fs,
        const TFsPath& dir,
        EBinaryClassifierType type,
        const TTokenEmbedder& tokenEmbedder,
        THashSet<TString>* setOfNames);
    static bool ShouldSelectClassifier(
        const TBinaryClassifierVariant& classifier,
        const TVector<TString>& enabledLstmClassifiers,
        const TVector<TString>& enabledDevClassifiers,
        bool enableBeggins,
        bool enableZeliboba,
        const TFreshOptions& freshOptions,
        EBinaryClassifierCollectionType collectionType);

private:
    TVector<TBinaryClassifierVariantPtr> Classifiers;
    TVector<TString> InitializationErrors;
};

// ~~~~ TSentenceFeaturesCalculator ~~~~

class TBinaryClassifiersSentenceFeaturesInfo {
public:
private:
    THashMap<TString, size_t> FeatureToIndex;
    THashMap<TString, TVector<size_t>> ClassifierToFeatures;
};

// ~~~~ TBegemotBinaryClassifiersCalculator ~~~~

class TBegemotBinaryClassifiersCalculator {
public:
    TBegemotBinaryClassifiersCalculator(
        const TTokenEmbedder* tokenEmbedder,
        const TBoltalkaDssmEmbedder* dssmEmbedder);

    void Load(const NBg::TFileSystem& fs);

    TVector<TString> EnumerateClassifiers(EBinaryClassifierCollectionType type) const;

    void Predict(
        const TString& request,
        const TVector<NGranet::TEntity>& entities,
        const TVector<float>& begginsEmbedding,
        const TVector<float>& zelibobaEmbedding,
        const TVector<TString>& enabledLstmClassifiers,
        const TVector<TString>& enabledDevClassifiers,
        const TFreshOptions& freshOptions,
        bool isLogEnabled,
        const NBg::TParallelExecutor& executor,
        THashMap<TString, float>* nameToProbability,
        TVector<TString>* log) const;

private:
    TMixedBinaryClassifierInput CreateInput(
        const TString& request,
        const TVector<NGranet::TEntity>& entities,
        const TVector<float>& begginsEmbedding,
        const TVector<float>& zelibobaEmbedding,
        const TVector<const TBinaryClassifierVariant*>& classifiers) const;
    static TString DumpSelectedClassifiers(const TVector<const TBinaryClassifierVariant*>& classifiers);

private:
    const TTokenEmbedder& TokenEmbedder;
    const TBoltalkaDssmEmbedder& DssmEmbedder;
    THashMap<EBinaryClassifierCollectionType, TBinaryClassifierCollection> ClassifierCollections;
};

} // namespace NAlice
