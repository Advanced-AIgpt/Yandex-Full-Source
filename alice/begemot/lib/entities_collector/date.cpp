#include "date.h"
#include "entities_collector.h"
#include <alice/nlu/granet/lib/sample/entity_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <library/cpp/scheme/scheme.h>

using namespace NNlu;
using namespace NGranet;

namespace NBg::NAliceEntityCollector {

static NSc::TValue ConvertDateBaseValue(const NProto::TExternalMarkupProto::TDate& date) {
    NSc::TValue value;
    if (date.HasYear()) {
        value["year"] = date.GetYear();
    }
    if (date.HasRelativeYear()) {
        value["year_is_relative"].SetBool(date.GetRelativeYear());
    }
    if (date.HasMonth()) {
        value["month"] = date.GetMonth();
    }
    if (date.HasRelativeMonth()) {
        value["month_is_relative"].SetBool(date.GetRelativeMonth());
    }
    if (date.HasDay()) {
        value["day"] = date.GetDay();
    }
    if (date.HasRelativeDay()) {
        value["day_is_relative"].SetBool(date.GetRelativeDay());
    }
    if (date.HasHour()) {
        value["hour"] = date.GetHour();
    }
    if (date.HasMin()) {
        value["minute"] = date.GetMin();
    }
    if (date.HasWeek()) {
        value["week"] = date.GetWeek();
    }
    if (date.HasRelativeWeek()) {
        value["week_is_relative"].SetBool(date.GetRelativeWeek());
    }
    return value;
}

static void ConvertDurationField(const TString& key, double value, bool isBack, NSc::TValue* dict) {
    (*dict)[key] = isBack ? -value : value;
    (*dict)[key + "_is_relative"].SetBool(true);
}

static NSc::TValue ConvertDurationValue(const NProto::TExternalMarkupProto::TDate::TDuration& duration) {
    NSc::TValue value;
    if (!duration.HasType()) {
        return value;
    }
    const bool isBack = duration.GetType() == TStringBuf("BACK");
    if (!isBack && duration.GetType() != TStringBuf("FORWARD")) {
        // PERIODICAL not supported
        return value;
    }
    if (duration.HasYear()) {
        ConvertDurationField("year", duration.GetYear(), isBack, &value);
    }
    if (duration.HasMonth()) {
        ConvertDurationField("month", duration.GetMonth(), isBack, &value);
    }
    if (duration.HasDay()) {
        ConvertDurationField("day", duration.GetDay(), isBack, &value);
    }
    if (duration.HasHour()) {
        ConvertDurationField("hour", duration.GetHour(), isBack, &value);
    }
    if (duration.HasMin()) {
        ConvertDurationField("minute", duration.GetMin(), isBack, &value);
    }
    if (duration.HasWeek()) {
        ConvertDurationField("week", duration.GetWeek(), isBack, &value);
    }
    return value;
}

static NSc::TValue ConvertDateValue(const NProto::TExternalMarkupProto::TDate& date) {
    NSc::TValue value = date.HasDuration()
        ? ConvertDurationValue(date.GetDuration())
        : ConvertDateBaseValue(date);

    // Convert weeks to days
    if (value.Has("week")) {
        value["day"] = value["day"].GetNumber(0) + value["week"].GetNumber() * 7;
        value.Delete("week");
    }
    if (value.Has("week_is_relative")) {
        value["day_is_relative"].SetBool(value["week_is_relative"].GetBool());
        value.Delete("week_is_relative");
    }
    return value;
}

TVector<NGranet::TEntity> CollectPASkillsDate(const NProto::TExternalMarkupProto& markup) {
    TVector<TEntity> entities;
    for (const NProto::TExternalMarkupProto::TDate& date : markup.GetDate()) {
        const NSc::TValue value = ConvertDateValue(date);
        if (value.IsNull()) {
            continue;
        }
        TEntity& entity = entities.emplace_back();
        entity.Interval = ToInterval(date.GetTokens());
        entity.Type = NEntityTypes::PA_SKILLS_DATETIME;
        entity.Value = value.ToJson();
        entity.LogProbability = NEntityLogProbs::PA_SKILLS_DATETIME;
    }
    return entities;
}

} // namespace NBg::NAliceEntityCollector
