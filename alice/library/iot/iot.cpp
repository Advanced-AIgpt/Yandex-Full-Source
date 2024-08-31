#include "iot.h"

#include "defs.h"
#include "indexer.h"
#include "preprocessor.h"
#include "priority.h"
#include "query_parser.h"
#include "utils.h"

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/subst.h>


namespace NAlice::NIot {

namespace {

using THypothesisToExtended = THashMap<NSc::TValue, NSc::TValue>;

const TVector<TString> NUM_TO_MODE = {
    "one",
    "two",
    "three",
    "four",
    "five",
    "six",
    "seven",
    "eight",
    "nine",
    "ten"
};

/* Entity types to which we pay attention when fill target devices */
const THashSet<TString> FILL_DEVICES_TARGET_ENTITY_TYPES = {
    ENTITY_TYPE_GROUPTYPE, ENTITY_TYPE_GROUP, ENTITY_TYPE_DEVICETYPE, ENTITY_TYPE_DEVICE, ENTITY_TYPE_CONJUNCTION
};

/* Entity types to which we pay attention when fill target rooms */
const THashSet<TString> FILL_ROOMS_TARGET_ENTITY_TYPES = {
    ENTITY_TYPE_GROUPTYPE, ENTITY_TYPE_GROUP, ENTITY_TYPE_DEVICETYPE, ENTITY_TYPE_DEVICE, ENTITY_TYPE_CONJUNCTION,
    ENTITY_TYPE_ROOMTYPE, ENTITY_TYPE_ROOM, ENTITY_TYPE_HOUSEHOLD, ENTITY_TYPE_HOUSEHOLDTYPE
};

const TVector<TString> TARGET_TYPES = {
    ENTITY_TYPE_DEVICE, ENTITY_TYPE_DEVICETYPE, ENTITY_TYPE_GROUP, ENTITY_TYPE_HOUSEHOLD, ENTITY_TYPE_ROOM
};

const THashSet<TString> FILL_HOUSEHOLDS_TARGET_ENTITY_TYPES = FILL_ROOMS_TARGET_ENTITY_TYPES;

constexpr TStringBuf EVERYWHERE_ROOM = "everywhere";

constexpr TStringBuf ALL_SPECIAL_MARK = "all";

template <typename Collection>
bool TryAddDeviceLikeIds(const NSc::TValue& applicableTypes, const TSmartHomeIndex& shIndex, const Collection& ids, NSc::TValue& entity,
                         NSc::TValue& hypothesis, TStringBuilder* logBuilder);

bool IsRelevantHypothesis(const NSc::TValue& extendedHypothesis);

// TODO(jan-fazli): Move more functions to this class
class THypothesesMaker {
public:
    THypothesesMaker(const TSmartHomeIndex& index,
                     const TExpFlags& expFlags,
                     const TIoTEntitiesInfo& entitiesInfo,
                     const ELanguage language,
                     TStringBuilder* logBuilder)
        : Language_(language)
        , Index(index)
        , ExpFlags(expFlags)
        , EntitiesInfo(entitiesInfo)
        , MaxPriority(std::numeric_limits<TPriorityType>::min())
        , LogBuilder(logBuilder)
    {}

    NSc::TValue Make();

private:
    void Log(const TString& toLog) const {
        IOT_LOG(LogBuilder, toLog);
    }

    template<typename TAction>
    bool TryResolveInstance(const TAction& action, const NSc::TValue& value,
                            const NSc::TValue& relative, NSc::TValue& result) const;
    bool TryMergeActionInstancesAndValues(NSc::TValue& rawParsingHypothesis) const;
    bool TryFillAction(const TParsingHypothesis& ph, NSc::TValue& hypothesis) const;

    bool TryFillTargetsFromParsed(const NSc::TValue& applicableTypes, TParsingHypothesis& ph, NSc::TValue& hypothesis) const;
    bool TryFillTargetsFromCapabilities(const NSc::TValue& applicableTypes, NSc::TValue& hypothesis) const;
    bool TryFillTargetsFromRoomsAndHouseholds(const NSc::TValue& applicableTypes, NSc::TValue& hypothesis) const;

    void AddHypothesis(NSc::TValue& hypothesis);

private:
    const ELanguage Language_;
    const TSmartHomeIndex& Index;
    const TExpFlags& ExpFlags;
    const TIoTEntitiesInfo& EntitiesInfo;

    THypothesisToExtended HypothesisToExtended;

    TPriorityType MaxPriority;

    mutable TStringBuilder* LogBuilder;
};

const NSc::TValue& GetSingleValue(const NSc::TValue& value) {
    return value.IsArray() ? value.Get(0) : value;
}

TVector<TString> GetDevicesForType(const TStringBuf type, const TSmartHomeIndex& shIndex) {
    if (const auto devicesIds = shIndex.DeviceTypeToDevicesIds.FindPtr(type)) {
        return TVector<TString>(devicesIds->cbegin(), devicesIds->cend());
    }
    return {};
}

bool TryCollectDevicesForType(const TStringBuf deviceType, const TSmartHomeIndex& shIndex, NSc::TValue& output) {
    const auto deviceIds = GetDevicesForType(deviceType, shIndex);
    output.AppendAll(deviceIds);
    return !deviceIds.empty();
}

template<class TRawEntities>
bool HasOriginalUnitForm(const TRawEntities& rawEntities, const ELanguage language) {
    const auto& originalUnitForms = GetPreprocessor(language).UnitWords;
    for (const auto& e : rawEntities) {
        if (e.Type() == ENTITY_TYPE_UNIT && originalUnitForms.contains(e.Text())) {
            return true;
        }
    }

    return false;
}

template<typename TAction>
bool IsQuery(const TAction& action) {
    return action.HasRequestType() && action.RequestType() == REQUEST_TYPE_QUERY;
}

bool IsQuery(const NSc::TValue& value) {
    return value.Has(FIELD_REQUEST_TYPE) && value.Get(FIELD_REQUEST_TYPE) == REQUEST_TYPE_QUERY;
}

// Pure targets are targets that should not have types and instances
bool IsPureTarget(const TString& target) {
    return target != TARGET_CAPABILITY && target != TARGET_PROPERTY;
}

template<typename TInfo>
bool IsValueAllowed(const TInfo& info, const NSc::TValue value) {
    return !info.HasAllowedValues()
        || AnyOf(info.AllowedValues(), [&](const auto& v){ return *v.GetRawValue() == value; });
}

template<typename TArgumentInfo>
bool TryFillArgument(const TParsingHypothesis& ph, const TArgumentInfo& argInfo,
                     NSc::TValue& result, const ELanguage language)
{
    if (argInfo.HasUnit()) {
        if (!ph.HasUnits() || ph.Units().Size() != 1 || ToString(ph.Units()[0]) != ToString(argInfo.Unit())) {
            return false;
        }
        result[FIELD_UNIT] = argInfo.Unit();

        if (!ph.HasArguments() && HasOriginalUnitForm(ph.RawEntities(), language) && result.Has(FIELD_RELATIVE)) {
            result[FIELD_VALUE].SetIntNumber(1);
            if (!IsValueAllowed(argInfo, result[FIELD_VALUE])) {
                return false;
            }
            return true;
        }
    } else if (ph.HasUnits()) {
        return false;
    }

    if (!ph.HasArguments()) {
        return false;
    }

    const auto& typeToParsedArgs = *ph.Arguments().GetRawValue();
    const auto& argsOfRequiredType = typeToParsedArgs.TrySelect(argInfo.Type());
    if (argsOfRequiredType.IsNull() || argsOfRequiredType.ArraySize() != 1 || typeToParsedArgs.DictSize() != 1) {
        return false;
    }

    const auto& value = argsOfRequiredType.Get(0);
    if (!IsValueAllowed(argInfo, value)) {
        return false;
    }
    if (result.TrySelect(FIELD_TYPE).GetString() == CAPABILITY_TYPE_MODE && argInfo.Type() == "num") {
        auto intValue = value.GetIntNumber();
        if (result.Has(FIELD_VALUE)) {
            result[FIELD_VALUE].SetString(TStringBuilder() << result[FIELD_VALUE].GetString() << "_" << intValue);
        } else if (intValue > 0 && static_cast<size_t>(intValue) <= NUM_TO_MODE.size()) {
            result[FIELD_VALUE] = NUM_TO_MODE[intValue - 1];
        } else {
            return false;
        }
    } else {
        result[FIELD_VALUE] = value;
    }

    return true;
}

template<typename TArgumentInfoVariants>
bool TryFillAnyArgument(const TParsingHypothesis& ph, const TArgumentInfoVariants& argInfoVariants,
                        NSc::TValue& result, const ELanguage language)
{
    for (const auto& argInfo : argInfoVariants) {
        if (TryFillArgument(ph, argInfo, result, language)) {
            return true;
        }
    }

    return false;
}

template<typename TAction>
bool TryResolveArguments(const TParsingHypothesis& ph, const TAction& action, NSc::TValue& result, const ELanguage language) {
    bool hasUnitsOrArguments = ph.HasUnits() || ph.HasArguments();

    if (action.HasAllowedArgument() && hasUnitsOrArguments) {
        return TryFillArgument(ph, action.AllowedArgument(), result, language);
    }
    if (action.HasAllowedArgumentVariants() && hasUnitsOrArguments) {
        return TryFillAnyArgument(ph, action.AllowedArgumentVariants(), result, language);
    }
    if (action.HasRequiredArgument()) {
        return TryFillArgument(ph, action.RequiredArgument(), result, language);
    }
    if (action.HasRequiredArgumentVariants()) {
        return TryFillAnyArgument(ph, action.RequiredArgumentVariants(), result, language);
    }

    return !hasUnitsOrArguments;
}

bool CheckValue(const NSc::TValue& value, const THashSet<TString>* const possibleValues, TStringBuilder* logBuilder = nullptr) {
    if (!possibleValues) {
        IOT_LOG(logBuilder, "No possible values");
        return false;
    }

    if (value.IsNull() || possibleValues->empty()) {
        return true;
    }

    const bool result = possibleValues->contains(value.ForceString());
    if (!result) {
        IOT_LOG_NONEWLINE(logBuilder, "Value not in possible values. Value: " << value.ForceString() << ". PossibleValues: [ ");
        for (const auto& v : *possibleValues) {
            IOT_LOG_NONEWLINE(logBuilder, v << " ");
        }
        IOT_LOG(logBuilder, "]");
    }
    return result;
}

template<typename TAction>
bool THypothesesMaker::TryResolveInstance(const TAction& action, const NSc::TValue& value,
                                          const NSc::TValue& relative, NSc::TValue& result) const {
    Log("Resolving action instance");

    if (action.HasInstance()) {
        result[FIELD_INSTANCE] = action.Instance();

        if (IsQuery(action)) {
            return true;
        }
        TString type = ToString(action.Type());
        TString instance = ToString(action.Instance());
        Log(TString::Join("Checking values for CapabilityTypeInstance ", type, "|", instance));
        auto possibleValues = Index.CapabilityTypeInstanceToValues.FindPtr(
            std::make_pair(std::move(type), std::move(instance))
        );
        return CheckValue(value, possibleValues, LogBuilder);
    }

    if (action.Type() != CAPABILITY_TYPE_MODE) {
        Log("Action has no instance, but is not a mode action");
        return false;
    }

    if (!relative.IsNull()) {
        result[FIELD_INSTANCES].AppendAll(Index.ModeInstances);
        const bool out = !Index.ModeInstances.empty();
        if (!out) {
            Log("Instanceless relative mode action has not instances in the indexer");
        }
        return out;
    }

    if (value.IsNull()) {
        Log("Instanceless non-relative mode action has no value");
        return false;
    }

    auto instances = Index.CapabilityTypeValueToInstances.FindPtr(
        std::make_pair(ToString(action.Type()), value.GetString())
    );
    if (!instances) {
        Log("Could not resolve instances by value for mode action");
        return false;
    }

    result[FIELD_INSTANCES].AppendAll(*instances);

    return true;
}

bool THypothesesMaker::TryFillAction(const TParsingHypothesis& ph, NSc::TValue& hypothesis) const {
    Log("Filling action");

    if (!ph.HasActions()) {
        Log("Hypothesis has no actions");
        return false;
    }
    if (ph.Actions().Size() > 1) {
        Log("Hypothesis has more than one action");
        return false;
    }

    const auto& action = ph.Actions()[0];
    bool hasPureTarget = false;
    if (action.HasTarget()) {
        if (!IsQuery(action)) {
            Log("Action has a target, but is not a query");
            return false;
        }
        hasPureTarget = IsPureTarget(TString{action.Target()});
    } else if (IsQuery(action)) {
        Log("Action has no target, but is a query");
        return false;
    }

    if (hasPureTarget && action.HasType()) {
        Log("Action has both a pure target and a type");
        return false;
    }
    if (!hasPureTarget && !action.HasType()) {
        Log("Action has no type but does not have a pure target");
        return false;
    }

    auto& result = hypothesis[FIELD_ACTION];
    if (action.HasType()) {
        result[FIELD_TYPE] = action.Type();
    }
    if (action.HasValue()) {
        result[FIELD_VALUE] = *action.Value().GetRawValue();
    }
    if (action.HasDefaultValue() && !result.Has(FIELD_VALUE)) {
        result[FIELD_VALUE] = *action.DefaultValue().GetRawValue();
    }
    if (result.Has(FIELD_VALUE) && !IsValueAllowed(action, result[FIELD_VALUE])) {
        Log("Result action has unallowed value");
        return false;
    }
    if (action.HasRelativeChange()) {
        result[FIELD_RELATIVE] = action.RelativeChange();
    }
    if (action.HasNLG()) {
        hypothesis[FIELD_NLG] = *action.NLG().GetRawValue();
    }
    hypothesis[FIELD_REQUEST_TYPE] = action.HasRequestType() ? action.RequestType() : REQUEST_TYPE_ACTION;
    if (IsQuery(action)) {
        result[FIELD_TARGET] = TString{action.Target()};
    }

    if (!TryResolveArguments(ph, action, result, Language_)) {
        Log("Could not resolve action arguments");
        return false;
    }

    // one of these fields should be filled
    const auto& value = result.TrySelect(FIELD_VALUE);
    const auto& relative = result.TrySelect(FIELD_RELATIVE);
    const bool noValueAndNotRelative = value.IsNull() && relative.IsNull();
    if (value.IsNull() && action.RequireValueWithRelative()) {
        Log("Got relative, but no value, which is required with param require_value_with_relative");
        return false;
    }
    if (noValueAndNotRelative && !IsQuery(action)) {
        Log("Non-query action has no value/relative");
        return false;
    }
    if (!noValueAndNotRelative && IsQuery(action)) {
        Log("Query action has a value/relative");
        return false;
    }

    if (hasPureTarget) {
        if (action.HasInstance()) {
            Log("Action with a pure target has an instance");
            return false;
        }
    } else if (!TryResolveInstance(action, value, relative, result)) {
        Log("Could not resolve action instance");
        return false;
    }

    bool hasSpecifiedDevices = ph.HasGroups() || ph.HasDevices() || ph.HasDeviceTypes();
    if (action.HasDefaultContext() && action.DefaultContext().HasDeviceType() && !hasSpecifiedDevices) {
        Log("Trying to collect devices for action from its default context");
        if (action.HasInstance() && action.HasType()) {
            NSc::TValue nullEntity;
            NSc::TValue applicableTypes;
            applicableTypes.Push(action.DefaultContext().DeviceType());
            if (!TryAddDeviceLikeIds(applicableTypes, Index, Index.DeviceIds, nullEntity, hypothesis, LogBuilder)) {
                Log("Failed using device type, action type and instance");
                return false;
            }
        } else if (!TryCollectDevicesForType(action.DefaultContext().DeviceType(), Index, hypothesis[FIELD_DEVICES])) {
            Log("Failed using just device type");
        }
    }

    if (action.RequireDeviceAsTarget() && !hasSpecifiedDevices && hypothesis.TrySelect(FIELD_DEVICES).IsNull()) {
        Log("Action requires device as target, but hypothesis has no devices");
        return false;
    }

    return true;
}

bool TryMakeScenarioHypothesis(const TParsingHypothesis& ph, NSc::TValue& result) {
    if (ph.HasRooms() ||
        ph.HasGroups() ||
        ph.HasDeviceTypes() ||
        ph.HasDevices() ||
        ph.HasArguments() ||
        ph.HasUnits() ||
        ph.Actions().Size() != 1
    ) {
        return false;
    }
    const auto& actionType = ph.Actions()[0].Type();
    if (ph.Scenarios().Size() == 1 && !ph.HasTriggeredScenarios() && actionType == "scenarios") {
        result[FIELD_SCENARIO].SetString(ph.Scenarios()[0]);
        return true;
    }
    if (ph.TriggeredScenarios().Size() == 1 && !ph.HasScenarios() && actionType == "triggered_scenarios") {
        result[FIELD_SCENARIO].SetString(ph.TriggeredScenarios()[0]);
        return true;
    }
    return false;
}

TVector<TString> GetIntersection(const TSmartHomeIndex& shIndex, const NSc::TValue& deviceType, const NSc::TValue& deviceIds) {
    auto idsForDeviceType = shIndex.DeviceTypeToDevicesAndGroupsIds.FindPtr(GetSingleValue(deviceType).GetString());
    if (!idsForDeviceType) {
        return {};
    }

    TVector<TString> intersectionIds;
    for (const auto& deviceId : deviceIds.GetArray()) {
        if (idsForDeviceType->contains(deviceId.GetString())) {
            intersectionIds.push_back(ToString(deviceId.GetString()));
        }
    }

    return intersectionIds;
}

TVector<NSc::TValue*> ExtractRawEntities(NSc::TValue& rawEntities, const THashSet<TString>& targetTypes) {
    TVector<NSc::TValue*> targetEntities;
    for (auto& entity : rawEntities.GetArrayMutable()) {
        if (targetTypes.contains(entity.TrySelect(FIELD_TYPE).GetString())) {
            targetEntities.push_back(&entity);
        }
    }
    return targetEntities;
}

bool IsApplicableDevice(const TString& id, const NSc::TValue& applicableTypes, const TSmartHomeIndex& shIndex) {
    if (applicableTypes.ArrayEmpty()) {
        return true;
    }

    auto deviceType = shIndex.DeviceLikeIdToType.FindPtr(id);
    Y_ENSURE(deviceType, "No device type in indexer for id " << id.Quote());

    for (const auto& type : applicableTypes.GetArray()) {
        if (type == DEVICE_TYPE_ALL) {
            return true;
        }
        if (IsSubType(*deviceType, type)) {
            return true;
        }
    }

    return false;
}

TString MultipleS(const size_t count) {
    return count == 1 ? "" : "s";
}

template <typename Collection>
bool TryAddDeviceLikeIds(const NSc::TValue& applicableTypes, const TSmartHomeIndex& shIndex, const Collection& ids, NSc::TValue& entity,
                         NSc::TValue& hypothesis, TStringBuilder* logBuilder) {
    const auto& action = hypothesis.Get(FIELD_ACTION);
    const auto& target = action.Get(FIELD_TARGET).ForceString(TARGET_CAPABILITY);
    const auto& value = action.Get(FIELD_VALUE);
    const auto& instance = action.Get(FIELD_INSTANCE).ForceString();
    const auto& type = action.Get(FIELD_TYPE).ForceString();

    const bool targetIsCapability = target == TARGET_CAPABILITY;
    const bool targetIsProperty = target == TARGET_PROPERTY;
    if (IsPureTarget(target)) {
        Y_ASSERT(!type && !instance);
    } else {
        Y_ASSERT(type && instance);
    }

    NSc::TValue idsToAdd;
    for (const auto& id : ids) {
        if (!IsApplicableDevice(id, applicableTypes, shIndex)) {
            IOT_LOG(logBuilder, "Device-like " << id << " is not applicable by types");
            continue;
        }

        if (targetIsCapability) {
            if (shIndex.DeviceLikeWithCapabilityInfo.contains(id)) {
                IOT_LOG(logBuilder, "Checking values for DeviceLikeCapabilityTypeInstance " << id << "|" << type << "|" << instance);
                auto possibleValues = shIndex.DeviceLikeCapabilityTypeInstanceToValues.FindPtr(
                    std::make_tuple(id, type, instance)
                );
                if (!CheckValue(value, possibleValues, logBuilder)) {
                    IOT_LOG(logBuilder, "Device-like " << id << " is not applicable by capability");
                    continue;
                }
            } else {
                IOT_LOG(logBuilder, "Device-like " << id << " is not applicable cause it has no capabilities");
                continue;
            }
        } else if (targetIsProperty) {
            if (!shIndex.DeviceLikePropertyTypeInstances.contains(std::make_tuple(id, type, instance))) {
                IOT_LOG(logBuilder, "Device-like " << id << " is not applicable by property");
                continue;
            };
        }

        idsToAdd.Push(id);
        IOT_LOG(logBuilder, "Device-like " << id << " is applicable");
    }

    if (idsToAdd.ArrayEmpty()) {
        IOT_LOG(logBuilder, "Could not add any device-like");
        return false;
    }

    for (const auto& id : idsToAdd.GetArray()) {
        if (IsIn(shIndex.DeviceIds, id)) {
            hypothesis[FIELD_DEVICES].Push(id);
        }
        if (IsIn(shIndex.GroupsIds, id)) {
            hypothesis[FIELD_GROUPS].Push(id);
        }
    }
    entity[FIELD_EXTRA][FIELD_IDS] = idsToAdd;

    IOT_LOG(logBuilder, "Added " << idsToAdd.ArraySize() << " device-like" << MultipleS(idsToAdd.ArraySize()));
    return true;
}

bool IsDeviceLike(const TStringBuf type) {
    return type == ENTITY_TYPE_DEVICE || type == ENTITY_TYPE_GROUP;
}

/* Here we try to assume that entity's type is device type and neighbor is a device-like entity (device or group). */
bool TryUseNeighbor(const NSc::TValue& applicableTypes, const TSmartHomeIndex& shIndex,
                    const NSc::TValue& entity, const size_t delta /* how much we should increase i if we found an itersection */,
                    size_t& leftEdge, size_t& i, NSc::TValue& neighbor, NSc::TValue& hypothesis,
                    TStringBuilder* logBuilder = nullptr) {
    auto type = entity.TrySelect(FIELD_TYPE).GetString();
    auto neighborType = neighbor.TrySelect(FIELD_TYPE).GetString();

    const auto& deviceTypeEntity = type == ENTITY_TYPE_DEVICETYPE ? entity : NSc::Null();
    const auto& deviceLikeEntity = IsDeviceLike(neighborType) ?  neighbor : NSc::Null();
    const auto intersection = deviceTypeEntity.IsNull() || deviceLikeEntity.IsNull() ? TVector<TString>() :
        GetIntersection(shIndex, deviceTypeEntity.TrySelect(FIELD_VALUE), deviceLikeEntity.TrySelect(FIELD_VALUE));
    if (intersection.size() > 0) {
        i += delta;
        leftEdge = i + 1;
        return TryAddDeviceLikeIds(applicableTypes, shIndex, intersection, neighbor, hypothesis, logBuilder);
    }

    return false;
}

TVector<TString> ToStrings(const NSc::TValue& values) {
    TVector<TString> result;
    for (const auto& item : values.GetArray()) {
        result.push_back(item.ForceString());
    }
    return result;
}

bool HasNeighborOfType(const TVector<NSc::TValue*>& entities, size_t i, size_t leftEdge, size_t rightEdge, const TString& type) {
    return i > leftEdge && entities[i - 1]->TrySelect(FIELD_TYPE).GetString() == type
        || i < rightEdge && entities[i + 1]->TrySelect(FIELD_TYPE).GetString() == type;
}


bool TryFillDevices(const NSc::TValue& applicableTypes, const TSmartHomeIndex& shIndex,
                    NSc::TValue& rawEntities, NSc::TValue& hypothesis, TStringBuilder* logBuilder = nullptr)
{
    const auto targetEntities = ExtractRawEntities(rawEntities, FILL_DEVICES_TARGET_ENTITY_TYPES);

    size_t leftEdge = 0; // We don't try to look at any indexes smaller than this
    size_t rightEdge = targetEntities.size() - 1;
    for (size_t i = 0; i < targetEntities.size(); ++i) {
        auto& entity = *targetEntities[i];
        const auto& value = entity.TrySelect(FIELD_VALUE);
        const auto& type = entity.TrySelect(FIELD_TYPE).GetString();

        IOT_LOG(logBuilder, "Filling devices from neighbors");
        if (i > leftEdge && TryUseNeighbor(applicableTypes, shIndex, entity, 0 /* delta */, leftEdge, i, *targetEntities[i - 1], hypothesis, logBuilder) ||
            i < rightEdge && TryUseNeighbor(applicableTypes, shIndex, entity, 1 /* delta */, leftEdge, i, *targetEntities[i + 1], hypothesis, logBuilder))
        {
            IOT_LOG(logBuilder, "Success");
            continue;
        }

        if (type == ENTITY_TYPE_DEVICETYPE) {
            const auto typeStr = GetSingleValue(value).GetString();
            IOT_LOG(logBuilder, "Filling devices for device type " << typeStr);
            auto idsForDeviceType = GetDevicesForType(typeStr, shIndex);
            if (idsForDeviceType.empty()) {
                IOT_LOG(logBuilder, "No such devices");
                return false;
            }
            if (!TryAddDeviceLikeIds(applicableTypes, shIndex, idsForDeviceType, entity, hypothesis, logBuilder)) {
                IOT_LOG(logBuilder, "Failure");
                return false;
            }
        } else if (type == ENTITY_TYPE_GROUPTYPE) {
            if (!HasNeighborOfType(targetEntities, i, leftEdge, rightEdge, ENTITY_TYPE_GROUP)) {
                IOT_LOG(logBuilder, "Got group_type entity with no group neighbor");
                return false;
            }
        } else if (type == ENTITY_TYPE_DEVICE || type == ENTITY_TYPE_GROUP) {
            const auto strs = ToStrings(value);
            IOT_LOG_NONEWLINE(logBuilder, "Filling devices for device" << MultipleS(strs.size()) << " or group" << MultipleS(strs.size()));
            for (const auto& str : strs) {
                IOT_LOG_NONEWLINE(logBuilder, " " << str);
            }
            IOT_LOG(logBuilder, "");
            if (!TryAddDeviceLikeIds(applicableTypes, shIndex, strs, entity, hypothesis, logBuilder)) {
                IOT_LOG(logBuilder, "Failure");
                return false;
            }
        }
    }

    return true;
}


bool TryFillHousehold(TParsingHypothesis& ph, NSc::TValue& hypothesis) {
    auto targetEntities = ExtractRawEntities(*ph.RawEntities().GetRawValue(), FILL_HOUSEHOLDS_TARGET_ENTITY_TYPES);
    size_t leftEdge = 0;
    size_t rightEdge = targetEntities.size() - 1;
    for (size_t i = 0; i < targetEntities.size(); ++i) {
        if (targetEntities[i]->TrySelect(FIELD_TYPE).GetString() == ENTITY_TYPE_HOUSEHOLDTYPE &&
            !HasNeighborOfType(targetEntities, i, leftEdge, rightEdge, ENTITY_TYPE_HOUSEHOLD))
        {
            return false;
        }
    }

    if (ph.HasHouseholdIds()) {
        hypothesis[FIELD_HOUSEHOLDS].AppendAll(ph.HouseholdIds().GetRawValue()->GetArray());
    }

    return true;
}


bool TryFillRooms(const TSmartHomeIndex& shIndex, const NSc::TValue& applicableTypes, TParsingHypothesis& ph, NSc::TValue& hypothesis) {
    auto targetEntities = ExtractRawEntities(*ph.RawEntities().GetRawValue(), FILL_ROOMS_TARGET_ENTITY_TYPES);
    size_t leftEdge = 0;
    size_t rightEdge = targetEntities.size() - 1;
    for (size_t i = 0; i < targetEntities.size(); ++i) {
        if (targetEntities[i]->TrySelect(FIELD_TYPE).GetString() == ENTITY_TYPE_ROOMTYPE &&
            !HasNeighborOfType(targetEntities, i, leftEdge, rightEdge, ENTITY_TYPE_ROOM))
        {
            return false;
        }
    }

    if (ph.HasRooms()) {
        hypothesis[FIELD_ROOMS].AppendAll(ph.Rooms().GetRawValue()->GetArray());

        if (!hypothesis.Has(FIELD_GROUPS) && !hypothesis.Has(FIELD_DEVICES) && !applicableTypes.ArrayEmpty()) {
            auto& devices = hypothesis[FIELD_DEVICES];
            for (const auto& type : applicableTypes.GetArray()) {
                auto ids = GetDevicesForType(type.GetString(), shIndex);
                devices.AppendAll(ids);
            }

            if (devices.ArrayEmpty()) {
                return false;
            }
        }
    }

    return true;
}

bool THypothesesMaker::TryFillTargetsFromParsed(const NSc::TValue& applicableTypes, TParsingHypothesis& ph, NSc::TValue& hypothesis) const {
    Log("Trying to fill targets from parsed");

    auto& rawEntities = hypothesis[FIELD_RAW_ENTITIES];
    for (auto& entity : rawEntities.GetArrayMutable()) {
        const auto& type = entity.TrySelect(FIELD_TYPE).GetString();
        if (type == ENTITY_TYPE_ROOM) {
            entity[FIELD_EXTRA][FIELD_IDS] = entity.TrySelect(FIELD_VALUE);
        }
    }

    return TryFillDevices(applicableTypes, Index, rawEntities, hypothesis, LogBuilder) &&
           TryFillRooms(Index, applicableTypes, ph, hypothesis) &&
           TryFillHousehold(ph, hypothesis);
}

bool TryFillDatetime(const TParsingHypothesis& ph, NSc::TValue& hypothesis) {
    if (!ph.HasFsts() || (!ph.Fsts().HasDatetimes() && !ph.Fsts().HasDatetimeRanges() && !ph.Fsts().HasTimes())) {
        return !ph.Actions(0).RequireTime();
    }

    // ALICE-8821. Queries can not have datetime specified for now anyway;
    if (ph.Actions().Empty() || ph.Actions(0).RequestType() != REQUEST_TYPE_ACTION) {
        return false;
    }

    NSc::TValue& datetime = hypothesis[FIELD_DATETIME];
    if (ph.Fsts().HasDatetimes()) {
        if (ph.Fsts().Datetimes().Size() != 1) {
            return false;
        }
        datetime = *ph.Fsts().Datetimes()[0].GetRawValue();
    }
    if (ph.Fsts().HasDatetimeRanges()) {
        if (ph.Fsts().DatetimeRanges().Size() != 1) {
            return false;
        }
        datetime = *ph.Fsts().DatetimeRanges()[0].GetRawValue();
    }
    if (ph.Fsts().HasTimes()) {
        if (ph.Fsts().Times().Size() != 1) {
            return false;
        }
        for (const auto& [field, value] : ph.Fsts().Times()[0].GetRawValue()->GetDict()) {
            if (datetime.Has(field)) {
                return false;
            }
            datetime[field] = value;
        }
    }

    return true;
}

void SortUnique(NSc::TValue& hypothesis) {
    for (auto& [field, value] : hypothesis.GetDictMutable()) {
        if (value.IsArray()) {
            SortUnique(value.GetArrayMutable(), [](const auto& a, const auto& b) { return a.GetString() < b.GetString(); });
        }
    }
}

void THypothesesMaker::AddHypothesis(NSc::TValue& hypothesis) {
    Log("Adding hypothesis");
    SortUnique(hypothesis);

    auto extendedHypothesis = hypothesis.Clone();
    auto priority = hypothesis.Delete(FIELD_PRIORITY).GetIntNumber(DEFAULT_PRIORITY);
    if (priority >= MaxPriority) {
        hypothesis.Delete(FIELD_RAW_ENTITIES);
        MaxPriority = priority;
        HypothesisToExtended[hypothesis] = extendedHypothesis;
        Log("Hypothesis added successfully");
    } else {
        Log(TString::Join("Hypothesis priority (", ToString(priority), ") is lower than max priority (", ToString(MaxPriority), ")"));
    }
}

bool TryMergeIntoValue(const NSc::TValue& parts, NSc::TValue& whole) {
    for (const auto& [key, value] : parts.GetDict()) {
        const auto& existingValue = whole.TrySelect(key);
        if (existingValue.IsNull()) {
            whole[key] = value;
        } else if (existingValue != value) {
            return false;
        }
    }

    return true;
}

bool THypothesesMaker::TryMergeActionInstancesAndValues(NSc::TValue& rawParsingHypothesis) const {
    const auto& actions = rawParsingHypothesis.TrySelect("action");
    if (actions.ArrayEmpty()) {
        Log("No actions in raw parsing hypothesis");
        return true;
    }

    const auto& instances = rawParsingHypothesis.TrySelect("instance");
    if (instances.ArraySize() > 1) {
        Log("Several instances in raw parsing hypothesis");
        return false;
    }

    const auto& values = rawParsingHypothesis.TrySelect("action_value");
    if (values.ArraySize() > 1) {
        Log("Several values in raw parsing hypothesis");
        return false;
    }

    auto& action = rawParsingHypothesis["action"][0];
    return TryMergeIntoValue(instances.TrySelect("[0]"), action) && TryMergeIntoValue(values.TrySelect("[0]"), action);
}

bool THypothesesMaker::TryFillTargetsFromCapabilities(const NSc::TValue& applicableTypes, NSc::TValue& hypothesis) const {
    Log("Trying to fill targets from capabilities");
    NSc::TValue nullEntity;
    return TryAddDeviceLikeIds(applicableTypes, Index, Index.DeviceIds, nullEntity, hypothesis, LogBuilder);
}

bool THypothesesMaker::TryFillTargetsFromRoomsAndHouseholds(const NSc::TValue& applicableTypes, NSc::TValue& hypothesis) const {
    Log("Trying to fill targets from rooms and households");
    const bool hasRooms = !hypothesis.TrySelect(FIELD_ROOMS).IsNull();
    const bool hasHouseholds = !hypothesis.TrySelect(FIELD_HOUSEHOLDS).IsNull();

    if (!hasRooms && !hasHouseholds) {
        Log("No rooms, no households, fallback to filling targets from capabilities");
        return TryFillTargetsFromCapabilities(applicableTypes, hypothesis);
    }
    const auto tryFillFrom = [&](const TStringBuf field, const THashMap<TString, THashSet<TString>>& index, const TStringBuf logName) {
        bool filled = false;
        NSc::TValue nullEntity;
        for (const auto& id : hypothesis.Get(field).GetArray()) {
            Log(TString::Join("Filling for ", logName, " ", TString{id}));
            const auto* devices = index.FindPtr(id);
            if (!devices) {
                Log(TString::Join("No devices in the ", logName));
            } else if (TryAddDeviceLikeIds(applicableTypes, Index, *devices, nullEntity, hypothesis, LogBuilder)) {
                filled = true;
            }
        }
        return filled;
    };
    if (hasRooms) {
        return tryFillFrom(FIELD_ROOMS, Index.RoomToDeviceIds, "room");
    } else if (hasHouseholds) {
        return tryFillFrom(FIELD_HOUSEHOLDS, Index.HouseholdToDeviceIds, "household");
    }
    Y_UNREACHABLE();
}

const NSc::TValue& GetApplicableTypes(const TParsingHypothesis& ph) {
    return ph.GetRawValue()->TrySelect("action/[0]/applicable_types");
}

bool ForceNoTarget(const TParsingHypothesis& ph) {
    return ph.GetRawValue()->TrySelect("action/[0]/force_no_target").GetBool(false);
}

bool AllowNoTarget(const TParsingHypothesis& ph) {
    return ph.GetRawValue()->TrySelect("action/[0]/allow_no_target").GetBool(false);
}

bool ShouldTryInferringTargetsFromCapabilities(const TParsingHypothesis& ph) {
    return ph.GetRawValue()->TrySelect("action/[0]/infer_targets_from_capabilities").GetBool(false);
}

bool ShouldTryInferringTargetsFromRoomsAndHouseholds(const TParsingHypothesis& ph) {
    return ph.GetRawValue()->TrySelect("action/[0]/infer_targets_from_rooms_and_households").GetBool(false);
}

THashSet<TString> InferRooms(const NSc::TValue& hypothesis, const TSmartHomeIndex& shIndex) {
    THashSet<TString> result;
    for (const auto& device : hypothesis[FIELD_DEVICES].GetArray()) {
        const auto* room = shIndex.DeviceIdToRoom.FindPtr(device.GetString());
        if (room) {
            result.insert(*room);
        }
    }

    for (const auto& group : hypothesis[FIELD_GROUPS].GetArray()) {
        const auto* rooms = shIndex.GroupIdToRooms.FindPtr(group.GetString());
        if (rooms) {
            for (const auto& room : *rooms) {
                result.insert(room);
            }
        }
    }

    return result;
}

template<class TRawEntities>
bool HasCommon(const TRawEntities& rawEntities) {
    return AnyOf(rawEntities, [](const auto& entity) {
        return entity.Type() == ENTITY_TYPE_COMMON;
    });
}

void HandleMultipleInstances(NSc::TValue& hypothesis, TVector<NSc::TValue>& resultHypotheses) {
    const auto& instances = hypothesis[FIELD_ACTION].Delete(FIELD_INSTANCES);
    if (instances.IsArray()) {
        ChangePriority(-NO_MATCHED_INSTANCE_PRIORITY_DECREASE, hypothesis);
        for (const auto& instance : instances.GetArray()) {
            auto instanceHypothesis = hypothesis.Clone();
            instanceHypothesis[FIELD_ACTION][FIELD_INSTANCE] = instance;
            resultHypotheses.push_back(instanceHypothesis);
        }
    } else {
        resultHypotheses.push_back(hypothesis);
    }
}


void RefineTime(NSc::TValue& hypothesis, const TVector<TRawEntity>& entities) {
    const bool hypothesisDoesntHaveTime = !hypothesis.Has(FIELD_DATETIME);
    const bool hypothesisDatetimeHasNoHour = !hypothesis.Get(FIELD_DATETIME).Has("hours");
    const bool hypothesisHourIsRelative = hypothesis.Get(FIELD_DATETIME).Has("hours_relative");
    const bool hasNoRawEntities = !hypothesis.Has(FIELD_RAW_ENTITIES);
    if (hypothesisDoesntHaveTime || hypothesisDatetimeHasNoHour || hypothesisHourIsRelative || hasNoRawEntities) {
        return;
    }

    auto& rawEntities = hypothesis.GetNoAdd(FIELD_RAW_ENTITIES)->GetArrayMutable();
    constexpr auto typeIsFstDatetime = [](const NSc::TValue& el) {return el.Get("type") == "fst_DATETIME";};
    auto* fstDatetimeRawEntity = FindIfPtr(rawEntities, typeIsFstDatetime);
    if (fstDatetimeRawEntity == nullptr) {
        return;
    }
    const int fstDatetimeStart = fstDatetimeRawEntity->Get("start").ForceIntNumber();
    const int fstDatetimeEnd = fstDatetimeRawEntity->Get("end").ForceIntNumber();
    const int fstDatetimeHours = fstDatetimeRawEntity->Get("value").Get("hours").ForceIntNumber();

    const NSc::TValue* fstTimeRawEntityValue = nullptr;
    for (int pos = fstDatetimeStart; pos < fstDatetimeEnd; ++pos) {
        for (const auto& entity : entities) {
            if (entity.AsEntity().GetStart() == pos &&
                entity.AsEntity().GetTypeStr() == "fst_TIME" &&
                entity.AsValue()["value"].Has("hours") &&
                (entity.AsValue()["value"].Get("hours").ForceIntNumber() - fstDatetimeHours) % 12 == 0)
            {
                if (fstTimeRawEntityValue != nullptr) {
                    // found multiple matches -> suspicious
                    return;
                }
                // time may be refined only if period's been found by fst_TIME
                if (entity.AsValue()["value"].Has("period")) {
                    fstTimeRawEntityValue = &entity.AsValue()["value"];
                }
            }
        }
    }
    if (fstTimeRawEntityValue == nullptr) {
        return;
    }

    for (NSc::TValue* beingCorrectedDatetimeValue : {fstDatetimeRawEntity->GetNoAdd("value"), hypothesis.GetNoAdd("datetime")}) {
        if (beingCorrectedDatetimeValue != nullptr) {
            beingCorrectedDatetimeValue->GetOrAdd("hours") = fstTimeRawEntityValue->Get("hours");
            beingCorrectedDatetimeValue->GetOrAdd("period") = fstTimeRawEntityValue->Get("period");
        }
    }
}

bool HasRoomAmongTargets(const TStringBuf roomId, const NSc::TValue& hypothesis, const TSmartHomeIndex& shIndex) {
    for (const auto& deviceId : hypothesis.Get(FIELD_DEVICES).GetArray()) {
        auto deviceRoomId = shIndex.DeviceIdToRoom.FindPtr(deviceId.GetString());
        if (deviceRoomId && roomId == *deviceRoomId) {
            return true;
        }
    }

    for (const auto& groupId : hypothesis.Get(FIELD_GROUPS).GetArray()) {
        auto groupRooms = shIndex.GroupIdToRooms.FindPtr(groupId.GetString());
        if (groupRooms && groupRooms->contains(roomId)) {
            return true;
        }
    }

    return false;
}

bool HasAllSpecialMark(const TParsingHypothesis& ph) {
    return IsIn(ph.SpecialMarks(), ALL_SPECIAL_MARK);
}


bool IsScenarioWithDatetimeRangeSpecified(const NSc::TValue& extendedHypothesis) {
    const bool isScenario = extendedHypothesis.Has(FIELD_SCENARIO);
    const auto& rawEntities = extendedHypothesis.Get(FIELD_RAW_ENTITIES).GetArray();
    const bool hasDatetimeRange = AnyOf(rawEntities, [](const NSc::TValue& entity) {
        return entity.GetDict().Get(FIELD_TYPE) == "fst_DATETIME_RANGE";
    });
    return isScenario && hasDatetimeRange;
}

bool IsShortCommandWithTvSynonym(const NSc::TValue& extendedHypothesis) {
    const bool actionTypeIsOnOff =
            extendedHypothesis.TrySelect("action/type").GetString() == "devices.capabilities.on_off";
    const bool actionValueIsTrue = extendedHypothesis.TrySelect("action/value").GetBool();
    const bool actionRelativeIsInvert = extendedHypothesis.TrySelect("action/relative").GetString() == "invert";

    const auto& rawEntities = extendedHypothesis.Get("raw_entities").GetArray();
    const bool usedTvSynonym = AnyOf(rawEntities, [&](const auto& rawEntity) {
        const bool keyIsTv = IsIn({"тв", "tv", "тиви"}, rawEntity.Get("text").GetString());
        const bool synonymWasUsed = rawEntity.TrySelect("extra/is_synonym").GetBool();
        const bool typeIsDevice = rawEntity.Get("type").GetString() == ENTITY_TYPE_DEVICE;

        return keyIsTv && synonymWasUsed && typeIsDevice;
    });

    const bool usedTvAsDeviceType = AnyOf(rawEntities, [&](const auto& entity) {
        const bool keyIsTv = IsIn({"тв", "tv", "тиви"}, entity.Get("text").GetString());
        const bool typeIsDeviceType = entity.Get("type").GetString() == ENTITY_TYPE_DEVICETYPE;
        return keyIsTv && typeIsDeviceType;
    });
    const bool noDeviceWasSpecifiedExplicitly = AllOf(rawEntities, [&](const auto& entity) {
        return entity.Get("type").GetString() != ENTITY_TYPE_DEVICE;
    });

    return ((usedTvAsDeviceType && noDeviceWasSpecifiedExplicitly) || usedTvSynonym) &&
            actionTypeIsOnOff &&
            (actionValueIsTrue || actionRelativeIsInvert);
}

bool IsRelevantHypothesis(const NSc::TValue& extendedHypothesis) {
    return !IsScenarioWithDatetimeRangeSpecified(extendedHypothesis) &&
           !IsShortCommandWithTvSynonym(extendedHypothesis);
}

bool HasSmartSpeakersAmongTargetDevices(const NSc::TValue& hypothesis, const TVector<TString>& smartSpeakersIds) {
    return AnyOf(hypothesis[FIELD_DEVICES].GetArray(), [&](const NSc::TValue& deviceId) {
        return IsIn(smartSpeakersIds, deviceId.GetString());
    });
}


NSc::TValue THypothesesMaker::Make() {
    Log("Making hypotheses");
    NSc::TValue result;
    result.SetArray();

    for (auto& rawParsingHypothesis : Parse(EntitiesInfo, Language_, LogBuilder)) {
        if (!TryMergeActionInstancesAndValues(rawParsingHypothesis)) {
            continue;
        }

        Log(TString::Join("Handling raw hypothesis: ", rawParsingHypothesis.ToJson()));
        TParsingHypothesis ph(&rawParsingHypothesis);

        if (ph.Actions(0).HasReplaceValueWith() && !ph.Actions(0).Value().IsNull()) {
            ph.Actions(0).Value() = *ph.Actions(0).ReplaceValueWith();
        }

        NSc::TValue preliminaryHypothesis;
        preliminaryHypothesis[FIELD_RAW_ENTITIES] = *ph.RawEntities().GetRawValue();

        ChangePriority(RawEntitiesCoverageToHypothesisPriorityChange(ph), preliminaryHypothesis);

        if (ExpFlags.contains("iot_forbid_common") && HasCommon(ph.RawEntities())) {
            Log("Hypothesis has forbidden common entities");
            continue;
        }

        if (!TryFillDatetime(ph, preliminaryHypothesis)) {
            Log("Failed filling Datetime for hypothesis");
            continue;
        }

        AddBonusForCorrectPrepositionBeforeDatetimeRange(ph, preliminaryHypothesis);

        if (ph.HasScenarios() || ph.HasTriggeredScenarios()) {
            Log("Hypothesis has scenarios");
            if (TryMakeScenarioHypothesis(ph, preliminaryHypothesis)) {
                AddHypothesis(preliminaryHypothesis);
            } else {
                Log("Failed making scenario hypothesis");
            }
            continue;
        }

        if (!TryFillAction(ph, preliminaryHypothesis)) {
            Log("Failed filling action");
            continue;
        }

        TVector<NSc::TValue> preliminaryHypotheses;
        HandleMultipleInstances(preliminaryHypothesis, preliminaryHypotheses);
        Log(TString::Join("Handling ", ToString(preliminaryHypotheses.size()), " instances"));

        const auto& applicableTypes = GetApplicableTypes(ph);
        IOT_LOG_NONEWLINE(LogBuilder, "Applicable types: [ ");
        for (const auto& type : applicableTypes.GetArray()) {
            IOT_LOG_NONEWLINE(LogBuilder, type << " ");
        }
        IOT_LOG(LogBuilder, "]");

        const auto hasAllSpecialMark = HasAllSpecialMark(ph);

        for (auto& hypothesis : preliminaryHypotheses) {
            Log(TString::Join("Handling hypothesis: ", hypothesis.ToJson()));

            if (!TryFillTargetsFromParsed(applicableTypes, ph, hypothesis)) {
                Log("Failed filling targets");
                continue;
            }

            const bool noTarget =
                hypothesis.TrySelect(FIELD_DEVICES).IsNull() &&
                hypothesis.TrySelect(FIELD_GROUPS).IsNull() &&
                hypothesis.TrySelect(FIELD_ROOMS).IsNull() &&
                hypothesis.TrySelect(FIELD_HOUSEHOLDS).IsNull();
            const bool forceNoTarget = ForceNoTarget(ph);

            if (!noTarget && forceNoTarget) {
                Log("Hypothesis has both a device/group/room and force_no_target");
                continue;
            }

            if (noTarget && !forceNoTarget && !AllowNoTarget(ph)) {
                if (!ShouldTryInferringTargetsFromCapabilities(ph)) {
                    Log("Hypothesis has no target but targets should not be inferred from capabilities");
                    continue;
                }
                if (!TryFillTargetsFromCapabilities(applicableTypes, hypothesis)) {
                    Log("Failed inferring targets from capabilities");
                    continue;
                }
            }

            if (Index.SmartHomesCount > 1) {
                ChangePriority(CalculatePriorityInMultiSmartHomeEnvironment(hypothesis, Index), hypothesis);
                Log("Changed priority because of the multi smart home environment");
            }

            if (!hasAllSpecialMark && Index.ClientRoom && hypothesis.Get(FIELD_ROOMS).IsNull() &&
                HasRoomAmongTargets(Index.ClientRoom, hypothesis, Index) && !IsQuery(hypothesis))
            {
                hypothesis[FIELD_ROOMS].Push(Index.ClientRoom);
                Log("Added client room to hypothesis");
            }

            if (hypothesis.TrySelect("action/relative").GetString() == "invert" &&
                hypothesis.TrySelect(FIELD_ROOMS).IsNull() && !hasAllSpecialMark &&
                InferRooms(hypothesis, Index).size() > 1)
            {
                Log("Inferred too many rooms for invert action");
                continue;
            }

            if (IsIn(hypothesis.Get(FIELD_ROOMS).GetArray(), EVERYWHERE_ROOM)) {
                if (hypothesis.Get(FIELD_ROOMS).ArraySize() > 1) {
                    Log("Hypothesis has both the Everywhere room and at least one other room");
                    continue;
                }
                hypothesis.Delete(FIELD_ROOMS);
                // Everywhere room can have the same name as user rooms, so we decrease its priority
                ChangePriority(-EVERYWHERE_ROOM_PRIORITY_DECREASE, hypothesis);
                Log("Changed priority because of the Everywhere room");
            }

            if (hypothesis.TrySelect(FIELD_DEVICES).IsNull() &&
                hypothesis.TrySelect(FIELD_GROUPS).IsNull() &&
                ShouldTryInferringTargetsFromRoomsAndHouseholds(ph) &&
                !TryFillTargetsFromRoomsAndHouseholds(applicableTypes, hypothesis))
            {
                Log("Failed inferring targets from rooms");
                continue;
            }

            if (!hasAllSpecialMark && Index.ClientHousehold && hypothesis.Get(FIELD_HOUSEHOLDS).IsNull()) {
                hypothesis[FIELD_HOUSEHOLDS].Push(Index.ClientHousehold);
                Log("Added client household to hypothesis");
            }

            if (ph.Actions(0).RequireSmartSpeakersAmongTargetDevices() &&
                !HasSmartSpeakersAmongTargetDevices(hypothesis, Index.SmartSpeakersIds))
            {
                Log("Skipping this hypothesis as no smart speakers are among target devices, which is required");
                continue;
            }

            if (const auto delta = ph.Actions(0).BonusIfMatchedRoom()) {
                if (AnyOf(hypothesis.Get(FIELD_ROOMS).GetArray(), [&](const auto& room) {
                    return HasRoomAmongTargets(room.GetString(), hypothesis, Index);
                })) {
                    ChangePriority(delta, hypothesis);
                }
            }

            AddBonus(ph, hypothesis);

            AddHypothesis(hypothesis);
        }
    }

    for (auto& [hypothesis, extendedHypothesis] : HypothesisToExtended) {
        auto priority = extendedHypothesis.Delete(FIELD_PRIORITY).GetIntNumber(DEFAULT_PRIORITY);

        Log(TString::Join("Got priority ", ToString(priority), " for hypothesis ", extendedHypothesis.ToJson()));
        const bool isRelevant = IsRelevantHypothesis(extendedHypothesis);
        if (!isRelevant) {
            Log("Hypothesis is irrelevant");
        } else if (priority == MaxPriority) {
            Log("Added hypothesis to result");
            result.Push(extendedHypothesis);
        } else {
            Log("Hypothesis has low priority");
        }
    }

    for (auto& hypothesis : result.GetArrayMutable()) {
        RefineTime(hypothesis, EntitiesInfo.Entities);
    }

    return result;
}

} // namespace

NSc::TValue MakeHypotheses(const TSmartHomeIndex& index,
                           const TIoTEntitiesInfo& entitiesInfo,
                           const ELanguage language,
                           const TExpFlags& expFlags,
                           TStringBuilder* logBuilder)
{
    return THypothesesMaker(index, expFlags, entitiesInfo, language, logBuilder).Make();
}

} // namespace NAlice::NIot
