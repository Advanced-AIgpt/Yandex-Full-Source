#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/library/experiments/flags.h>
#include <alice/library/logger/logger.h>
#include <alice/library/music/defs.h>

#include <alice/protos/data/language/language.pb.h>

#include <util/string/cast.h>
#include <util/string/join.h>


namespace NAlice::NHollywood::NMusic::NImpl {

const TStringBuf DEFAULT_AMBIENT_SOUND = "playlist/103372440:1919";
const TStringBuf DEFAULT_AMBIENT_SOUND_TYPE = "playlist";

const THashMap<TStringBuf, TStringBuf> PLAYBACK_TYPE_TO_OBJECT_TYPE = {
    {"track", "Track"},
    {"album", "Album"},
    {"artist", "Artist"},
    {"playlist", "Playlist"},
};

class TAmbientSoundMusicPlayPatcher {
public:
    TAmbientSoundMusicPlayPatcher(TFrame& frame, const TScenarioRunRequestWrapper& request, TRTLogger& logger)
        : Frame_(frame)
        , Logger_(logger)
        , AmbientSound_(Frame_.FindSlot(NAlice::NMusic::SLOT_AMBIENT_SOUND))
        , EnabledByExperiment_(IsEnabledByExperiment(request))
    {
    }

    bool Patch() && {
        if (!AmbientSound_) {
            return false;
        }

        if (!EnabledByExperiment_) {
            LOG_INFO(Logger_) << "Converting ambient_sound slot to search_text slot, "
                                 "since haven't found 'hw_music_enable_ambient_sound' flag";
            SlotPatches_.clear();
            AddSlotPatch(NAlice::NMusic::SLOT_SEARCH_TEXT, AmbientSound_->Value.AsString());
        } else {
            LeaveOnlyRelevantSlots();

            if (AmbientSound_->Type == NAlice::NMusic::SLOT_AMBIENT_SOUND_TYPE) {
                const auto [objectType, objectId] = CollectPlaybackTypeAndObjectId();
                LOG_INFO(Logger_) << "Have received ambient_sound slot filled with a typed entity. "
                                     "Patching object_id and object_type slots with " << objectId << " and " << objectType;
                AddSlotPatch(NAlice::NMusic::SLOT_OBJECT_TYPE, objectType);
                AddSlotPatch(NAlice::NMusic::SLOT_OBJECT_ID, objectId);
            } else {
                LOG_INFO(Logger_) << "Have received ambient_sound slot filled with an untyped entity. "
                                     "Moving its value to the search_text slot";
                AddSlotPatch(NAlice::NMusic::SLOT_SEARCH_TEXT, AmbientSound_->Value.AsString());
            }
        }

        ApplyPatchesToFrame();

        Frame_.RemoveSlots(NAlice::NMusic::SLOT_AMBIENT_SOUND);

        return EnabledByExperiment_;
    }

private:
    void AddSlotPatch(const TStringBuf slotName, const TStringBuf slotValue) {
        SlotPatches_.emplace_back(slotName, slotValue);
    }

    std::pair<TString, TString> CollectPlaybackTypeAndObjectId() {
        const TStringBuf ambientSoundValue = AmbientSound_->Value.AsString();
        TStringBuf playbackType, objectId;
        if (!ambientSoundValue.TrySplit('/', playbackType, objectId)) {
            LOG_INFO(Logger_) << "Found invalid value in custom.ambient_sound entity: " << ambientSoundValue
                             << "; replacing it with default playlist:" << DEFAULT_AMBIENT_SOUND;
            objectId = DEFAULT_AMBIENT_SOUND;
            playbackType = DEFAULT_AMBIENT_SOUND_TYPE;
        }

        if (!PLAYBACK_TYPE_TO_OBJECT_TYPE.contains(playbackType)) {
            LOG_INFO(Logger_) << "Found unknown object_type in custom.ambient_sound entity: " << playbackType
                             << "; replacing it with default playlist:" << DEFAULT_AMBIENT_SOUND;
            objectId = DEFAULT_AMBIENT_SOUND;
            playbackType = DEFAULT_AMBIENT_SOUND_TYPE;
        }

        return std::make_pair(ToString(PLAYBACK_TYPE_TO_OBJECT_TYPE.at(playbackType)), ToString(objectId));
    }

    void ApplyPatchesToFrame() {
        for (const auto& [slotName, slotValue] : SlotPatches_) {
            if (auto slot = Frame_.FindSlot(slotName)) {
                LOG_INFO(Logger_) << "Found unexpected slots with name " << slotName << " (" << slot->Value.AsString() << ") "
                                  << "with non-empty ambient_sound slot";
                Frame_.RemoveSlots(slotName);
            }
            Frame_.AddSlot(TSlot{
                .Name = TString(slotName),
                .Type = "string",
                .Value = TSlot::TValue(ToString(slotValue)),
            });
        }
    }

    void LeaveOnlyRelevantSlots() {
        static const TVector<TStringBuf> relevantSlotNames = {
            NAlice::NMusic::SLOT_AMBIENT_SOUND,
            NAlice::NMusic::SLOT_ACTION_REQUEST,
            NAlice::NMusic::SLOT_LOCATION,
        };

        THashSet<TString> foundIrrelevantSlotNames;
        for (const auto& slot : Frame_.Slots()) {
            if (!IsIn(relevantSlotNames, slot.Name)) {
                foundIrrelevantSlotNames.insert(slot.Name);
            }
        }

        if (!foundIrrelevantSlotNames.empty()) {
            LOG_INFO(Logger_) << "Found unexpected slots with names [" << JoinSeq(", ", foundIrrelevantSlotNames) << "] "
                              << "along with non-empty ambient_sound slot; removing them";
            for (const auto& slotName : foundIrrelevantSlotNames) {
                Frame_.RemoveSlots(slotName);
            }
        }
    }

    static bool IsEnabledByExperiment(const TScenarioRunRequestWrapper& request) {
        return (
            request.HasExpFlag(NAlice::NExperiments::EXP_HW_MUSIC_ENABLE_AMBIENT_SOUND) ||
            request.Proto().GetBaseRequest().GetUserLanguage() == NAlice::ELang::L_ARA
        );
    }

private:
    TFrame& Frame_;
    TRTLogger& Logger_;
    TVector<std::pair<TString, TString>> SlotPatches_ = {
        {ToString(NAlice::NMusic::SLOT_REPEAT), "All"},
        {ToString(NAlice::NMusic::SLOT_ORDER), "shuffle"},
    };

    TPtrWrapper<TSlot> AmbientSound_;
    const bool EnabledByExperiment_;
};

};
