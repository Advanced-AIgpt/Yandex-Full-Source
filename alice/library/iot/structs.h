#pragma once

#include "defs.h"
#include <alice/library/iot/scheme.sc.h>

#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/megamind/protos/scenarios/iot.pb.h>
#include <alice/protos/data/device/info.pb.h>
#include <alice/protos/data/location/group.pb.h>
#include <alice/protos/data/location/room.pb.h>

#include <google/protobuf/util/message_differencer.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <util/string/join.h>


namespace NAlice::NIot {

// TODO(jan-fazli): Move somewhere
#define IOT_LOG_NONEWLINE(iotLogBuilder, x) if (iotLogBuilder) { *iotLogBuilder << x; }  // Cout << x;
#define IOT_LOG(iotLogBuilder, x) IOT_LOG_NONEWLINE(iotLogBuilder, x); IOT_LOG_NONEWLINE(iotLogBuilder, Endl);

using TParsingHypothesis = NIOTScheme::TParsingHypothesis<TSchemeTraits>;
using TRawParsingHypotheses = TVector<NSc::TValue>;


class TRawEntity {
public:
    TRawEntity(NSc::TValue value, TString type, TString text,
               size_t start, size_t end, NSc::TValue extra)
    {
        Entity_.SetTypeStr(type);
        Entity_.SetText(text);
        Entity_.SetStart(static_cast<int>(start));
        Entity_.SetEnd(static_cast<int>(end));
        Entity_.SetValue(value.ToJsonSafe());
        Entity_.MutableExtra()->AddIsCloseVariation(extra["is_close_variation"].GetBool());
        Entity_.MutableExtra()->AddIsSynonym(extra["is_synonym"].GetBool());
        Entity_.MutableExtra()->SetIsExact(extra["is_exact"].GetBool());
        if (extra["ids"].IsArray()) {
            *Entity_.MutableExtra()->MutableIds() = {extra["ids"].GetArray().begin(), extra["ids"].GetArray().end()};
        } else if (const auto idString = extra["ids"].ForceString(); !idString.empty()) {
            *Entity_.MutableExtra()->AddIds() = idString;
        }
        InitValueFromEntity();
    }

    TRawEntity(const NAlice::NScenarios::TIotEntity& entity)
        : Entity_(entity)
    {
        InitValueFromEntity();
    }

    const NAlice::NScenarios::TIotEntity& AsEntity() const {
        return Entity_;
    }

    const NSc::TValue& AsValue() const {
        return Value_;
    }

    bool IsDemo() const {
        const auto& value = Value_["value"];
        constexpr auto isDemoValue = [](const auto& value) { return value.GetString().StartsWith(DEMO_PREFIX); };
        return (!value.ArrayEmpty() && AllOf(value.GetArray(), isDemoValue)) || isDemoValue(value);
    }

    bool operator==(const TRawEntity& other) const {
        return ::google::protobuf::util::MessageDifferencer::Equals(Entity_, other.Entity_);
    }

    static TRawEntity FromValue(NSc::TValue value) {
        return TRawEntity(
            value["value"],
            ToString(value["type"].GetString()),
            ToString(value["text"].GetString()),
            value["start"].GetIntNumber(),
            value["end"].GetIntNumber(),
            value["extra"]
        );
    }

private:
    void InitValueFromEntity() {
        Value_.Clear();
        Value_["value"] = NSc::TValue::FromJson(Entity_.GetValue());
        Value_["type"].SetString(Entity_.GetTypeStr());
        Value_["text"].SetString(Entity_.GetText());
        Value_["start"].SetIntNumber(Entity_.GetStart());
        Value_["end"].SetIntNumber(Entity_.GetEnd());

        if (!Entity_.GetExtra().GetIsSynonym().empty() && Entity_.GetExtra().GetIsSynonym(0)) {
            Value_["extra"]["is_synonym"] = true;
        }
        if (!Entity_.GetExtra().GetIsCloseVariation().empty() && Entity_.GetExtra().GetIsCloseVariation(0)) {
            Value_["extra"]["is_close_variation"] = true;
        }
        if (Entity_.GetExtra().GetIsExact()) {
            Value_["extra"]["is_exact"] = true;
        }
        for (const auto& id : Entity_.GetExtra().GetIds()) {
            Value_["extra"]["ids"].Push(id);
        }
    }

private:
    NAlice::NScenarios::TIotEntity Entity_;
    NSc::TValue Value_;
};

using TRawEntityMatches = TVector<TVector<TRawEntity>>;

class TUniqueEntities {
public:
    void Add(const TRawEntity& entity) {
        if (!IsIn(Entities_, entity)) {
            Entities_.push_back(entity);
        }
    }

    const TVector<TRawEntity>& Get() const {
        return Entities_;
    }
private:
    TVector<TRawEntity> Entities_;
};

struct TNluInput {
    struct TExtra {
        TVector<TString> Tokens;
        TVector<double> NonsenseProbabilities;
    };

    explicit TNluInput(TString&& utterance)
        : Utterance(std::move(utterance))
    {
    }

    TNluInput(TVector<TString>&& tokens, TVector<double>&& nonsenseProbabilities)
        : Utterance(JoinSeq(" ", tokens))
        , Extra{std::move(tokens), std::move(nonsenseProbabilities)}
    {
    }

    TString Utterance;
    TExtra Extra;
};

struct TIoTEnvironment {
    TVector<const TIoTUserInfo*> SmartHomes;
    TString ClientDeviceId = "";
};

} // namespace NAlice::NIot
