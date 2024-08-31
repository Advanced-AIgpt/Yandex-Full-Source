#pragma once

#include "news_fast_data.h"

#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/json/json.h>
#include <alice/memento/proto/api.pb.h>
#include <alice/memento/proto/user_configs.pb.h>

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>

using namespace ru::yandex::alice::memento::proto;

namespace NAlice::NHollywood {

constexpr TStringBuf MEMENTO_IS_ONBOARDED = "is_onboarded";
constexpr TStringBuf MEMENTO_RESULT = "result";
constexpr TStringBuf MEMENTO_RUBRIC = "rubric";
constexpr TStringBuf MEMENTO_SOURCE = "source";

constexpr TStringBuf MEMENTO_RESULT_ANOTHER_SCENARIO = "another_scenario";
constexpr TStringBuf MEMENTO_RESULT_EMPTY = "empty";
constexpr TStringBuf MEMENTO_RESULT_ERROR = "error";
constexpr TStringBuf MEMENTO_RESULT_SUCCESS = "success";

constexpr TStringBuf MEMENTO_DEFAULT_SOURCE_ID = "6e24a5bb-yandeks-novost";
constexpr TStringBuf MEMENTO_SOURCE_DEFAULT = "default";

class TMementoHelper {
public:
    TMementoHelper(const TNewsFastData& fastData);
    NJson::TJsonValue ParseMementoReponse(const NScenarios::TMementoData& mementoData, bool ignoreMementoResponse);
    void PrepareConfig(NScenarios::TMementoData& mementoData, const TString& rubric, const TString& source);
    bool PrepareChangeDefaultRequest(TString topic, NScenarios::TMementoData& mementoData);
    TString GetSourceName(TString topic);
    bool IsMementableTopic(TString topic);

private:
    const TNewsFastData& FastData;
};

void AddMementoChangeUserObjectsDirective(TResponseBodyBuilder& bodyBuilder, const NScenarios::TMementoData& mementoData);

} // namespace NAlice::NHollywood
