#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>


namespace NAlice::NIot {

constexpr TStringBuf ARG_PREFIX = "arg_";
constexpr TStringBuf BOW_PREFIX = "bow_";
constexpr TStringBuf COMPLEX_PREFIX = "complex_";
constexpr TStringBuf FST_PREFIX = "fst_";

constexpr TStringBuf FIELD_ACTION = "action";
constexpr TStringBuf FIELD_ALIASES = "aliases";
constexpr TStringBuf FIELD_CAPABILITIES = "capabilities";
constexpr TStringBuf FIELD_COLORS = "colors";
constexpr TStringBuf FIELD_DATETIME = "datetime";
constexpr TStringBuf FIELD_DEVICES = "devices";
constexpr TStringBuf FIELD_EXTRA = "extra";
constexpr TStringBuf FIELD_GROUPS = "groups";
constexpr TStringBuf FIELD_ID = "id";
constexpr TStringBuf FIELD_IDS = "ids";
constexpr TStringBuf FIELD_INSTANCE = "instance";
constexpr TStringBuf FIELD_INSTANCES = "instances";
constexpr TStringBuf FIELD_NAME = "name";
constexpr TStringBuf FIELD_NLG = "nlg";
constexpr TStringBuf FIELD_PRIORITY = "priority";
constexpr TStringBuf FIELD_RAW_ENTITIES = "raw_entities";
constexpr TStringBuf FIELD_RELATIVE = "relative";
constexpr TStringBuf FIELD_REQUEST_TYPE = "request_type";
constexpr TStringBuf FIELD_ROOMS = "rooms";
constexpr TStringBuf FIELD_HOUSEHOLDS = "households";
constexpr TStringBuf FIELD_SCENARIO = "scenario";
constexpr TStringBuf FIELD_SCENARIO_TRIGGERS = "triggers";
constexpr TStringBuf FIELD_SCENARIOS = "scenarios";
constexpr TStringBuf FIELD_TARGET = "target";
constexpr TStringBuf FIELD_TYPE = "type";
constexpr TStringBuf FIELD_UNIT = "unit";
constexpr TStringBuf FIELD_VALUE = "value";
constexpr TStringBuf FIELD_VALUES = "values";

const TString ENTITY_TYPE_COMMON = "common";
const TString ENTITY_TYPE_CONJUNCTION = "conjunction";
const TString ENTITY_TYPE_DEVICE = "device";
const TString ENTITY_TYPE_DEVICETYPE = "device_type";
const TString ENTITY_TYPE_GROUP = "group";
const TString ENTITY_TYPE_GROUPTYPE = "group_type";
const TString ENTITY_TYPE_ROOM = "room";
const TString ENTITY_TYPE_ROOMTYPE = "room_type";
const TString ENTITY_TYPE_HOUSEHOLD = "household";
const TString ENTITY_TYPE_HOUSEHOLDTYPE = "household_type";
const TString ENTITY_TYPE_SCENARIO = "scenario";
const TString ENTITY_TYPE_TRIGGERED_SCENARIO = "triggered_scenario";
const TString ENTITY_TYPE_UNIT = "unit";

constexpr TStringBuf CAPABILITY_TYPE_MODE = "devices.capabilities.mode";
constexpr TStringBuf CAPABILITY_TYPE_ON_OFF = "devices.capabilities.on_off";
constexpr TStringBuf CAPABILITY_TYPE_COLOR_SETTING = "devices.capabilities.color_setting";
constexpr TStringBuf CAPABILITY_TYPE_RANGE = "devices.capabilities.range";
constexpr TStringBuf CAPABILITY_TYPE_TOGGLE = "devices.capabilities.toggle";
constexpr TStringBuf CAPABILITY_TYPE_CUSTOM_BUTTON = "devices.capabilities.custom.button";

constexpr TStringBuf DEVICE_TYPE_ALL = "all";

constexpr TStringBuf SMART_SPEAKER_TYPE = "devices.types.smart_speaker";
constexpr TStringBuf CAR_TYPE = "devices.types.remote_car";

constexpr TStringBuf DEMO_PREFIX = "demo--";

constexpr TStringBuf REQUEST_TYPE_ACTION = "action";
constexpr TStringBuf REQUEST_TYPE_QUERY = "query";

const TString TARGET_CAPABILITY = "capability";
const TString TARGET_PROPERTY = "property";

constexpr TStringBuf FLAG_DO_NOT_RECOVER_DATETIME_RANGE = "do_not_recover_datetime_range";

} // namespace NAlice::NIot
