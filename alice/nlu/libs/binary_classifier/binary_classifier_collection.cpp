#include "binary_classifier_collection.h"

#include <alice/begemot/lib/fresh_options/fresh_options.h>
#include <alice/nlu/granet/lib/utils/utils.h>

#include <library/cpp/iterator/zip.h>
#include <util/generic/is_in.h>
#include <util/generic/serialized_enum.h>
#include <util/string/cast.h>
#include <util/string/join.h>

namespace NAlice {

namespace {

constexpr TStringBuf LSTM_MODELS_DIR = "lstm_models";
constexpr TStringBuf DSSM_MODELS_DIR = "dssm_models";
constexpr TStringBuf BEGGINS_MODELS_DIR = "beggins_models";
constexpr TStringBuf ZELIBOBA_MODELS_DIR = "zeliboba_models";
constexpr TStringBuf MIXED_MODELS_DIR = "mixed_models";

constexpr TStringBuf MODEL_DESCRIPTION_FILE = "model_description.json";
constexpr TStringBuf NN_MODEL_FILE = "model.pb";
constexpr TStringBuf CATBOOST_MODEL_FILE = "model.cbm";

THolder<IInputStream> RequireInputStream(const NBg::TFileSystem& fs, const TFsPath& path) {
    THolder<IInputStream> stream = fs.LoadInputStream(path);
    Y_ENSURE(stream, "Can't find file " << path.GetName().Quote());
    return stream;
}

} // namespace

// ~~~~ TBinaryClassifierVariant ~~~~

TBinaryClassifierVariant::TBinaryClassifierVariant(const TString& name)
    : Name(name)
{
}

const TString& TBinaryClassifierVariant::GetName() const {
    return Name;
}

const TFsPath& TBinaryClassifierVariant::GetModelPath() const {
    return ModelPath;
}

void TBinaryClassifierVariant::Load(const NBg::TFileSystem& fs, const TFsPath& path,
    EBinaryClassifierType type, const TTokenEmbedder& tokenEmbedder)
{
    switch (type) {
        case BCT_LSTM_CLASSIFIER:
            LoadLstmClassifier(fs, path, tokenEmbedder);
            break;
        case BCT_DSSM_CLASSIFIER:
            LoadDssmClassifier(fs, path);
            break;
        case BCT_BEGGINS_CLASSIFIER:
            LoadBegginsClassifier(fs, path);
            break;
        case BCT_ZELIBOBA_CLASSIFIER:
            LoadZelibobaClassifier(fs, path);
            break;
        case BCT_MIXED_CLASSIFIER:
            LoadMixedClassifier(fs, path);
            break;
        default:
            Y_ENSURE(false);
    }
}

void TBinaryClassifierVariant::LoadLstmClassifier(const NBg::TFileSystem& fs, const TFsPath& path,
    const TTokenEmbedder& tokenEmbedder)
{
    Y_ENSURE(!IsInitialized());
    ModelPath = path;
    THolder<IInputStream> modelStream = RequireInputStream(fs, path / NN_MODEL_FILE);
    THolder<IInputStream> descriptionStream = RequireInputStream(fs, path / MODEL_DESCRIPTION_FILE);
    LstmClassifier = MakeHolder<TLstmBasedBinaryClassifier>(tokenEmbedder, modelStream.Get(), descriptionStream.Get());
}

void TBinaryClassifierVariant::LoadDssmClassifier(const NBg::TFileSystem& fs, const TFsPath& path) {
    Y_ENSURE(!IsInitialized());
    ModelPath = path;
    THolder<IInputStream> modelStream = RequireInputStream(fs, path / NN_MODEL_FILE);
    THolder<IInputStream> descriptionStream = RequireInputStream(fs, path / MODEL_DESCRIPTION_FILE);
    DssmClassifier = MakeHolder<TDssmBasedBinaryClassifier>(modelStream.Get(), descriptionStream.Get());
}

void TBinaryClassifierVariant::LoadBegginsClassifier(const NBg::TFileSystem& fs, const TFsPath& path) {
    Y_ENSURE(!IsInitialized());
    ModelPath = path;
    if (fs.Exists(path / CATBOOST_MODEL_FILE)) {
        THolder<IInputStream> modelStream = RequireInputStream(fs, path / CATBOOST_MODEL_FILE);
        BegginsClassifier = MakeHolder<TBegginsCatBoostBinaryClassifier>(modelStream.Get());
    } else {
        THolder<IInputStream> modelStream = RequireInputStream(fs, path / NN_MODEL_FILE);
        THolder<IInputStream> descriptionStream = RequireInputStream(fs, path / MODEL_DESCRIPTION_FILE);
        BegginsClassifier = MakeHolder<TBegginsTensorflowBinaryClassifier>(modelStream.Get(), descriptionStream.Get());
    }
}

void TBinaryClassifierVariant::LoadZelibobaClassifier(const NBg::TFileSystem& fs, const TFsPath& path) {
    Y_ENSURE(!IsInitialized());
    ModelPath = path;
    if (fs.Exists(path / CATBOOST_MODEL_FILE)) {
        THolder<IInputStream> modelStream = RequireInputStream(fs, path / CATBOOST_MODEL_FILE);
        ZelibobaClassifier = MakeHolder<TBegginsCatBoostBinaryClassifier>(modelStream.Get());
    } else {
        THolder<IInputStream> modelStream = RequireInputStream(fs, path / NN_MODEL_FILE);
        THolder<IInputStream> descriptionStream = RequireInputStream(fs, path / MODEL_DESCRIPTION_FILE);
        ZelibobaClassifier = MakeHolder<TBegginsTensorflowBinaryClassifier>(modelStream.Get(), descriptionStream.Get());
    }
}

void TBinaryClassifierVariant::LoadMixedClassifier(const NBg::TFileSystem& fs, const TFsPath& path) {
    Y_ENSURE(!IsInitialized());
    ModelPath = path;
    THolder<IInputStream> modelStream = RequireInputStream(fs, path / NN_MODEL_FILE);
    THolder<IInputStream> descriptionStream = RequireInputStream(fs, path / MODEL_DESCRIPTION_FILE);
    MixedClassifier = MakeHolder<TMixedBinaryClassifier>(modelStream.Get(), descriptionStream.Get());
}

bool TBinaryClassifierVariant::IsInitialized() const {
    return DssmClassifier != nullptr ||
        LstmClassifier != nullptr ||
        BegginsClassifier != nullptr ||
        ZelibobaClassifier != nullptr ||
        MixedClassifier != nullptr;
}

bool TBinaryClassifierVariant::IsLstmClassifier() const {
    return LstmClassifier != nullptr;
}

bool TBinaryClassifierVariant::IsDssmClassifier() const {
    return DssmClassifier != nullptr;
}

bool TBinaryClassifierVariant::IsBegginsClassifier() const {
    return BegginsClassifier != nullptr;
}

bool TBinaryClassifierVariant::IsZelibobaClassifier() const {
    return ZelibobaClassifier != nullptr;
}

bool TBinaryClassifierVariant::IsMixedClassifier() const {
    return MixedClassifier != nullptr;
}

bool TBinaryClassifierVariant::NeedSentenceEmebedding(TStringBuf embeddingName) const {
    if (LstmClassifier != nullptr) {
        return false;
    }
    if (DssmClassifier != nullptr) {
        return embeddingName == DSSM_EMBEDDING_NAME;
    }
    if (BegginsClassifier != nullptr) {
        return embeddingName == BEGGINS_EMBEDDING_NAME;
    }
    if (ZelibobaClassifier != nullptr) {
        return embeddingName == ZELIBOBA_EMBEDDING_NAME;
    }
    if (MixedClassifier != nullptr) {
        return MixedClassifier->NeedSentenceEmebedding(embeddingName);
    }
    Y_ENSURE(false);
    return false;
}

bool TBinaryClassifierVariant::NeedSentenceFeatures() const {
    if (MixedClassifier != nullptr) {
        return MixedClassifier->NeedSentenceFeatures();
    }
    return false;
}

float TBinaryClassifierVariant::Predict(const TMixedBinaryClassifierInput& input) const {
    Y_ENSURE(IsInitialized());
    if (LstmClassifier != nullptr) {
        return LstmClassifier->PredictProbability(input.Text);
    }
    if (DssmClassifier != nullptr) {
        const TVector<float>* embedding = input.SentenceEmbeddings.FindPtr(DSSM_EMBEDDING_NAME);
        if (embedding == nullptr) {
            return 0;
        }
        return DssmClassifier->PredictProbability(*embedding);
    }
    if (BegginsClassifier != nullptr) {
        const TVector<float>* embedding = input.SentenceEmbeddings.FindPtr(BEGGINS_EMBEDDING_NAME);
        if (embedding == nullptr) {
            return 0;
        }
        return BegginsClassifier->PredictProbability(*embedding);
    }
    if (ZelibobaClassifier != nullptr) {
        const TVector<float>* embedding = input.SentenceEmbeddings.FindPtr(ZELIBOBA_EMBEDDING_NAME);
        if (embedding == nullptr) {
            return 0;
        }
        return ZelibobaClassifier->PredictProbability(*embedding);
    }
    if (MixedClassifier != nullptr) {
        return MixedClassifier->Predict(input);
    }
    return 0;
}

// ~~~~ TBinaryClassifierCollection ~~~~

void TBinaryClassifierCollection::Load(const NBg::TFileSystem& fs, const TFsPath& dir,
    const TTokenEmbedder& tokenEmbedder)
{
    if (!fs.Exists(dir)) {
        InitializationErrors.push_back("Cannot find models directory " + dir.GetName().Quote());
        return;
    }
    THashSet<TString> setOfNames;
    LoadClassifiersOfType(fs, dir / LSTM_MODELS_DIR, BCT_LSTM_CLASSIFIER, tokenEmbedder, &setOfNames);
    LoadClassifiersOfType(fs, dir / DSSM_MODELS_DIR, BCT_DSSM_CLASSIFIER, tokenEmbedder, &setOfNames);
    LoadClassifiersOfType(fs, dir / BEGGINS_MODELS_DIR, BCT_BEGGINS_CLASSIFIER, tokenEmbedder, &setOfNames);
    LoadClassifiersOfType(fs, dir / ZELIBOBA_MODELS_DIR, BCT_ZELIBOBA_CLASSIFIER, tokenEmbedder, &setOfNames);
    LoadClassifiersOfType(fs, dir / MIXED_MODELS_DIR, BCT_MIXED_CLASSIFIER, tokenEmbedder, &setOfNames);
}

void TBinaryClassifierCollection::LoadClassifiersOfType(
    const NBg::TFileSystem& fs,
    const TFsPath& dir,
    EBinaryClassifierType type,
    const TTokenEmbedder& tokenEmbedder,
    THashSet<TString>* setOfNames)
{
    if (!fs.Exists(dir)) {
        return;
    }

    const THolder<NBg::TFileSystem> dirFs = fs.Subdirectory(dir);
    Y_ENSURE(dirFs);

    for (const TString& name : dirFs->List()) {
        try {
            const auto [_, inserted] = setOfNames->emplace(name);
            Y_ENSURE(inserted, "Multiple models for " << name.Quote() << " intent found");

            TBinaryClassifierVariantPtr classifier = MakeHolder<TBinaryClassifierVariant>(name);
            classifier->Load(fs, dir / name, type, tokenEmbedder);
            Classifiers.push_back(std::move(classifier));
        } catch (const yexception& e) {
            InitializationErrors.push_back(e.what());
        }
    }
}

const TVector<TString>& TBinaryClassifierCollection::GetInitializationErrors() const {
    return InitializationErrors;
}

TVector<TString> TBinaryClassifierCollection::EnumerateClassifiers() const {
    TVector<TString> result;
    for (const TBinaryClassifierVariantPtr& classifier : Classifiers) {
        result.push_back(classifier->GetName());
    }
    return result;
}

void TBinaryClassifierCollection::Select(
    const TVector<TString>& enabledLstmClassifiers,
    const TVector<TString>& enabledDevClassifiers,
    bool enableBeggins,
    bool enableZeliboba,
    const TFreshOptions& freshOptions,
    EBinaryClassifierCollectionType collectionType,
    TVector<const TBinaryClassifierVariant*>* selected) const
{
    Y_ENSURE(selected);
    for (const TBinaryClassifierVariantPtr& classifier : Classifiers) {
        if (ShouldSelectClassifier(*classifier, enabledLstmClassifiers, enabledDevClassifiers,
            enableBeggins, enableZeliboba, freshOptions, collectionType))
        {
            selected->push_back(classifier.Get());
        }
    }
}

// static
bool TBinaryClassifierCollection::ShouldSelectClassifier(
    const TBinaryClassifierVariant& classifier,
    const TVector<TString>& enabledLstmClassifiers,
    const TVector<TString>& enabledDevClassifiers,
    bool enableBeggins,
    bool enableZeliboba,
    const TFreshOptions& freshOptions,
    EBinaryClassifierCollectionType collectionType)
{
    const TString& name = classifier.GetName();

    switch (collectionType) {
        case BCCT_STATIC:
            if (ShouldUseFreshForForm(freshOptions, name)) {
                return false;
            }
            break;
        case BCCT_FRESH:
            if (!ShouldUseFreshForForm(freshOptions, name)) {
                return false;
            }
            break;
        case BCCT_DEV:
            if (!IsIn(enabledDevClassifiers, name)) {
                return false;
            }
            break;
        default:
            Y_ENSURE(false);
            break;
    }

    if (classifier.IsLstmClassifier()) {
        return IsIn(enabledLstmClassifiers, name);
    } else if (classifier.IsDssmClassifier()) {
        return true;
    } else if (classifier.IsBegginsClassifier()) {
        return enableBeggins;
    } else if (classifier.IsZelibobaClassifier()) {
        return enableZeliboba;
    } else if (classifier.IsMixedClassifier()) {
        return true;
    } else {
        Y_ENSURE(false);
    }

    return false;
}

// ~~~~ TBegemotBinaryClassifiersCalculator ~~~~

TBegemotBinaryClassifiersCalculator::TBegemotBinaryClassifiersCalculator(
        const TTokenEmbedder* tokenEmbedder,
        const TBoltalkaDssmEmbedder* dssmEmbedder)
    : TokenEmbedder(*tokenEmbedder)
    , DssmEmbedder(*dssmEmbedder)
{
    Y_ENSURE(tokenEmbedder);
    Y_ENSURE(dssmEmbedder);
}

void TBegemotBinaryClassifiersCalculator::Load(const NBg::TFileSystem& fs) {
    for (const auto type : GetEnumAllValues<EBinaryClassifierCollectionType>()) {
        const TString dir = ToString(type);
        ClassifierCollections[type].Load(fs, dir, TokenEmbedder);
    }
}

TVector<TString> TBegemotBinaryClassifiersCalculator::EnumerateClassifiers(EBinaryClassifierCollectionType type) const {
    const TBinaryClassifierCollection* collection = ClassifierCollections.FindPtr(type);
    if (collection == nullptr) {
        return {};
    }
    return collection->EnumerateClassifiers();
}

void TBegemotBinaryClassifiersCalculator::Predict(
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
    TVector<TString>* log) const
{
    Y_ENSURE(nameToProbability);

    if (log) {
        for (const auto& [type, collection] : ClassifierCollections) {
            NGranet::Extend(collection.GetInitializationErrors(), log);
        }
    }

    const bool enableBeggins = !begginsEmbedding.empty();
    const bool enableZeliboba = !zelibobaEmbedding.empty();
    TVector<const TBinaryClassifierVariant*> classifiers;
    for (const auto& [type, collection] : ClassifierCollections) {
        collection.Select(enabledLstmClassifiers, enabledDevClassifiers, enableBeggins,
            enableZeliboba, freshOptions, type, &classifiers);
    }

    if (log && isLogEnabled) {
        log->push_back(DumpSelectedClassifiers(classifiers));
    }

    const TMixedBinaryClassifierInput input = CreateInput(request, entities, begginsEmbedding, zelibobaEmbedding, classifiers);

    TVector<float> probabilities(classifiers.size(), 0.f);
    TVector<TString> errors(classifiers.size());

    executor.ForExecute(classifiers.size(), [&](size_t i) {
        try {
            probabilities[i] = classifiers[i]->Predict(input);
        } catch (const yexception& e) {
            errors[i] = TStringBuilder() << "Error while executing classifier "
                << classifiers[i]->GetName().Quote() << ": " << e.what();
        }
    });

    for (const auto& [classifier, probability, error] : Zip(classifiers, probabilities, errors)) {
        (*nameToProbability)[classifier->GetName()] = probability;
        if (error && log) {
            log->push_back(error);
        }
    }
}

TMixedBinaryClassifierInput TBegemotBinaryClassifiersCalculator::CreateInput(
    const TString& request,
    const TVector<NGranet::TEntity>& entities,
    const TVector<float>& begginsEmbedding,
    const TVector<float>& zelibobaEmbedding,
    const TVector<const TBinaryClassifierVariant*>& classifiers) const
{
    TMixedBinaryClassifierInput input;
    input.Text = request;
    if (!begginsEmbedding.empty()) {
        input.SentenceEmbeddings[BEGGINS_EMBEDDING_NAME] = begginsEmbedding;
    }
    if (!zelibobaEmbedding.empty()) {
        input.SentenceEmbeddings[ZELIBOBA_EMBEDDING_NAME] = zelibobaEmbedding;
    }

    const bool needDssmEmbedding = AnyOf(classifiers, [](const TBinaryClassifierVariant* classifier) {
        return classifier->NeedSentenceEmebedding(DSSM_EMBEDDING_NAME);
    });
    if (needDssmEmbedding) {
        input.SentenceEmbeddings[DSSM_EMBEDDING_NAME] = DssmEmbedder.Embed(request);
    }

    const bool needFeatures = AnyOf(classifiers, [](const TBinaryClassifierVariant* classifier) {
        return classifier->NeedSentenceFeatures();
    });
    if (needFeatures) {
        input.SentenceFeatures = CalculateSentenceFeatures(request, entities);
    }

    return input;
}

// static
TString TBegemotBinaryClassifiersCalculator::DumpSelectedClassifiers(const TVector<const TBinaryClassifierVariant*>& classifiers) {
    TStringBuilder out;
    out << "Selected classifiers:" << Endl;
    for (const TBinaryClassifierVariant* classifier : classifiers) {
        out << "  " << classifier->GetModelPath() << Endl;
    }
    return out;
}

} // namespace NAlice
