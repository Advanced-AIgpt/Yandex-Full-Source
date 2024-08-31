#include "memento_helper.h"

#include <alice/protos/data/news_provider.pb.h>

namespace NAlice::NHollywood {

namespace {

const THashMap<TString, TString> RUBRIC_NAMES = {
    {"index", "Яндекс Новости"},
    {"politics", "новости политики"},
    {"society", "новости общества"},
    {"business", "новости бизнеса"},
    {"world", "мировые новости"},
    {"sport", "новости спорта"},
    {"incident", "новости происшествий"},
    {"culture", "новости культуры"},
    {"computers", "новости технологий"},
    {"science", "новости науки"},
    {"auto", "новости авто"},
    {"personal", "персональные новости"},
};

} // anon namespace

TMementoHelper::TMementoHelper(const TNewsFastData& fastData) : FastData(fastData) {
}

NJson::TJsonValue TMementoHelper::ParseMementoReponse(const NScenarios::TMementoData& mementoData, bool ignoreMementoResponse)
{
    NJson::TJsonValue result;
    if (ignoreMementoResponse) {
        result[MEMENTO_RESULT] = MEMENTO_RESULT_ERROR;
        return result;
    }

    TNewsConfig newsConfig = mementoData.GetUserConfigs().GetNewConfig();

    NData::TNewsProvider defaultProvider;
    bool initialized = false;
    for (const TNewsConfig::TNewsConfigProviderPair& pair : newsConfig.GetNewsConfig()) {
        if (pair.GetNewsConfigType() == TNewsConfig::ENewsConfigType::TNewsConfig_ENewsConfigType_DEFAULT_GENERAL_PROVIDER) {
            defaultProvider = pair.GetNewsProvider();
            initialized = true;
            break;
        }
    }
    if(!initialized || newsConfig.GetIsNew()) {
        result[MEMENTO_RESULT] = MEMENTO_RESULT_EMPTY;
        return result;
    }

    if (defaultProvider.GetNewsSource() == MEMENTO_DEFAULT_SOURCE_ID) {
        result[MEMENTO_SOURCE] = MEMENTO_SOURCE_DEFAULT;
    } else if (!FastData.HasMementoId(defaultProvider.GetNewsSource())) {
        result[MEMENTO_RESULT] = MEMENTO_RESULT_ANOTHER_SCENARIO;
        return result;
    } else {
        result[MEMENTO_SOURCE] = FastData.GetSmiByMementoId(defaultProvider.GetNewsSource())->GranetId;
    }
    result[MEMENTO_RUBRIC] = defaultProvider.GetRubric();
    result[MEMENTO_RESULT] = MEMENTO_RESULT_SUCCESS;
    result[MEMENTO_IS_ONBOARDED] = newsConfig.GetIsOnboarded();
    return result;
}

void TMementoHelper::PrepareConfig(NScenarios::TMementoData& mementoData, const TString& rubric, const TString& source) {
    TNewsConfig* config = mementoData.MutableUserConfigs()->MutableNewConfig();
    const bool isOnboarded = config->GetIsOnboarded();
    config->Clear();
    config->SetIsOnboarded(isOnboarded);
    TNewsConfig::TNewsConfigProviderPair* providerPair = config->AddNewsConfig();
    providerPair->SetNewsConfigType(TNewsConfig::ENewsConfigType::TNewsConfig_ENewsConfigType_DEFAULT_GENERAL_PROVIDER);
    NData::TNewsProvider* newsProvider = providerPair->MutableNewsProvider();
    newsProvider->SetNewsSource(source);
    newsProvider->SetRubric(rubric);
}

bool TMementoHelper::PrepareChangeDefaultRequest(TString topic, NScenarios::TMementoData& mementoData) {
    TString rubric;
    TString source;

    const TSmi* smi = FastData.GetSmiByGranetId(topic);
    if (smi && FastData.IsMementableSmi(topic)) {
        source = smi->MementoId;
        rubric = "";
    }
    else if (RUBRIC_NAMES.contains(topic)) {
        source = MEMENTO_DEFAULT_SOURCE_ID;
        rubric = topic;
    }
    else {
        return false;
    }

    PrepareConfig(mementoData, rubric, source);
    return true;
}

TString TMementoHelper::GetSourceName(TString topic) {
    if (RUBRIC_NAMES.contains(topic)) {
        return RUBRIC_NAMES.at(topic);
    }
    const TSmi* smi = FastData.GetSmiByGranetId(topic);
    if (smi) {
        return smi->NameAccusative;
    }
    return {};
}

bool TMementoHelper::IsMementableTopic(TString topic) {
    return RUBRIC_NAMES.contains(topic) || FastData.IsMementableSmi(topic);
}

void AddMementoChangeUserObjectsDirective(TResponseBodyBuilder& bodyBuilder, const NScenarios::TMementoData& mementoData) {
    auto* directive = bodyBuilder.GetResponseBody().AddServerDirectives();
    auto* mementoDirective = directive->MutableMementoChangeUserObjectsDirective();

    auto* userObjects = mementoDirective->MutableUserObjects();

    using namespace ru::yandex::alice::memento::proto;
    auto* pair = userObjects->AddUserConfigs();
    pair->SetKey(EConfigKey::CK_NEWS);
    ::google::protobuf::Any config_any;
    config_any.PackFrom(mementoData.GetUserConfigs().GetNewConfig());
    *pair->MutableValue() = config_any;
}

} // namespace NAlice::NHollywood
