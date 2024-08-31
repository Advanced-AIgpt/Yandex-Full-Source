#pragma once

#include "vins.h"

#include <alice/bass/libs/radio/fmdb.h>

namespace NBASS {

namespace NRadio {
inline constexpr TStringBuf YA_RADIO_SLOT_NAME = "ya_radio";
inline constexpr TStringBuf FM_RADIO_SLOT_NAME = "fm_radio";
inline constexpr TStringBuf FM_FREQ_SLOT_NAME = "fm_freq";
inline constexpr TStringBuf FM_RADIO_FREQ_SLOT_NAME = "fm_radio_freq";
inline constexpr TStringBuf FM_RADIO_INFO_SLOT_NAME = "fm_radio_info";

inline constexpr TStringBuf TYPE_KNOWN_RADIO = "fm_radio_station";
inline constexpr TStringBuf TYPE_KNOWN_RADIO_FREQ = "fm_radio_freq";
inline constexpr TStringBuf TYPE_KNOWN_RADIO_INFO = "fm_radio_info";

inline constexpr TStringBuf ATTENTION_COUNTRY_IS_NOT_SUPPORTED = "country_is_not_supported";
inline constexpr TStringBuf ATTENTION_NO_FM_STATIONS_IN_REGION = "no_fm_stations_in_region";
inline constexpr TStringBuf ATTENTION_FM_STATION_IS_TEMPORARY_UNAVAILABLE = "fm_station_is_temporary_unavailable";
inline constexpr TStringBuf ATTENTION_FM_STATION_IS_UNAVAILABLE = "fm_station_is_unavailable";
inline constexpr TStringBuf ATTENTION_FM_STATION_IS_UNRECOGNIZED = "fm_station_is_unrecognized";
inline constexpr TStringBuf ATTENTION_NO_FM_STATION = "no_fm_station";
inline constexpr TStringBuf ATTENTION_NO_REGION_FM_DB = "no_region_fm_db";
inline constexpr TStringBuf ATTENTION_NO_STATION = "no_station";
inline constexpr TStringBuf ATTENTION_ONBOARDING_NO_MORE_STATIONS = "onboarding_no_more_stations";
inline constexpr TStringBuf ATTENTION_OPEN_RADIOSTATION_WEBSITE = "open_radiostation_website";
inline constexpr TStringBuf ATTENTION_RESTRICTED_BY_CHILD_CONTENT_SETTINGS = "station_restricted_by_child_content_settings";
inline constexpr TStringBuf ATTENTION_STATION_NOT_FOUND_LAUNCH_PERSONAL = "station_not_found_launch_personal";
inline constexpr TStringBuf ATTENTION_STATION_NOT_FOUND_LAUNCH_RECOMMENDED = "station_not_found_launch_recommended";
inline constexpr TStringBuf ATTENTION_USE_LONG_INTRO = "use_long_intro";
inline constexpr TStringBuf ATTENTION_UNSUPPORTED_USER_REGION = "unsupported_user_region";
inline constexpr TStringBuf ATTENTION_SIMPLE_NLU = "simple_nlu";

inline constexpr TStringBuf QUASAR_RADIO_PLAY_OBJECT_ACTION_NAME = "quasar.radio_play_object";

enum class ESelectionMethod {
    Any,
    Current,
    Next,
    Previous
};

TMaybe<TString> GetRadioByFreq(TContext& ctx, const double radioFreq, const NAutomotive::TFMRadioDatabase& radioDatabase);
TResultValue FetchOrderedRadioList(TContext& ctx, TVector<NSc::TValue>& resultList);

} // namespace NRadio

class TRadioFormHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
    static TContext::TPtr SetAsResponse(TContext& ctx, bool callbackSlot);
    static TResultValue SearchRadioStream(TContext& ctx, const TStringBuf desiredRadioId, NRadio::ESelectionMethod selectionMethod, TMaybe<NSc::TValue>& streamData);
    static TResultValue SearchRadioStream(TContext& ctx, const THashSet<TStringBuf>& desiredRadioIds, NRadio::ESelectionMethod selectionMethod, TMaybe<NSc::TValue>& streamData);
    static TResultValue HandleRadioStream(TContext& ctx, const TStringBuf desiredRadioId, NRadio::ESelectionMethod selectionMethod, const bool overrideRadioIdIfChild = true, TStringBuf alarmId = {});
    static TResultValue HandleRadioStream(TContext& ctx, const THashSet<TStringBuf>& desiredRadioIds, NRadio::ESelectionMethod selectionMethod, const bool overrideRadioIdIfChild = true, TStringBuf alarmId = {});
    static TMaybe<TString> GetRadioName(TContext& ctx, const bool isFmRadio, const TContext::TSlot* slotFmRadio, const TContext::TSlot* slotFmRadioFreq, const NAutomotive::TFMRadioDatabase& radioDatabase);
    static TMaybe<TStringBuf> GetRadioId(const TMaybe<TString>& namedRadioStation);
    static TResultValue LaunchRadio(TContext& ctx, const TMaybe<TStringBuf>& radioId, NRadio::ESelectionMethod selectionMethod);
    static TStringBuf GetCurrentlyPlayingRadioId(const NSc::TValue* radioState);

private:
    static THolder<NAutomotive::TFMRadioDatabase> RadioDatabase;
};

class TRadioPlayObjectActionHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

} // namespace NBASS
