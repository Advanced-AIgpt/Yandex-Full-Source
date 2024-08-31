#include "converter.h"

namespace {
    constexpr TStringBuf IS_GENERAL = "isGeneral";
    constexpr TStringBuf FILTERS = "filters";
    constexpr TStringBuf OBJECT = "object";
    constexpr TStringBuf ID = "id";
    constexpr TStringBuf TYPE = "type";
    constexpr TStringBuf MODIFIERS = "modifiers";
} // namespace

bool QuasarRequestConverter::Convert() {
    if (RequestBody.IsNull()) {
        return false;
    }

    auto& musicPlayTsf = *TypedSemanticFrame.MutableMusicPlaySemanticFrame();
    musicPlayTsf.MutableDisableNlg()->SetBoolValue(true);

    if (!RequestBody[IS_GENERAL].IsNull()) {
        if (!RequestBody[IS_GENERAL].IsBool()) {
            return false;
        }
        if (RequestBody[IS_GENERAL].GetBool()) {
            // Default semantic frame turning on user:onyouwave
            return true;
        }
    }

    if (!RequestBody[FILTERS].IsNull() && !RequestBody[FILTERS][OBJECT].IsNull()) {
        return ConvertObject();
    }

    return false;
}

void QuasarRequestConverter::SetAlarmId(const TStringBuf alarmId) {
    auto& musicPlayTsf = *TypedSemanticFrame.MutableMusicPlaySemanticFrame();
    musicPlayTsf.MutableAlarmId()->SetStringValue(alarmId.data());
}


bool QuasarRequestConverter::ConvertObject() {
    const auto& object = RequestBody[FILTERS][OBJECT];

    if (!object[TYPE].IsString() || !object[ID].IsString())
    {
        return false;
    }
    auto& musicPlayTsf = *TypedSemanticFrame.MutableMusicPlaySemanticFrame();

    const TStringBuf type = object[TYPE].GetString();
    if (type == "track") {
        musicPlayTsf.MutableObjectType()->SetEnumValue(NAlice::TMusicPlayObjectTypeSlot::Track);
    } else if (type == "album") {
        musicPlayTsf.MutableObjectType()->SetEnumValue(NAlice::TMusicPlayObjectTypeSlot::Album);
    } else if (type == "artist") {
        musicPlayTsf.MutableObjectType()->SetEnumValue(NAlice::TMusicPlayObjectTypeSlot::Artist);
    } else if (type == "playlist" || type == "specialPlaylist") {
        musicPlayTsf.MutableObjectType()->SetEnumValue(NAlice::TMusicPlayObjectTypeSlot::Playlist);
    } else {
        return false;
    }

    musicPlayTsf.MutableObjectId()->SetStringValue(object[ID].GetString().data());

    return ApplyModifiers();
}

bool QuasarRequestConverter::ApplyModifiers() {
    const auto& modifiers = RequestBody[MODIFIERS];

    if (!modifiers.IsNull()) {
        auto& musicPlayTsf = *TypedSemanticFrame.MutableMusicPlaySemanticFrame();

        if (!modifiers["shuffle"].IsNull()) {
            if (!modifiers["shuffle"].IsBool()) {
                return false;
            }
            if (modifiers["shuffle"].GetBool()) {
                musicPlayTsf.MutableOrder()->SetOrderValue("shuffle");
            }
        }

        if (!modifiers["repeat"].IsNull()) {
            if (!modifiers["repeat"].IsBool()) {
                return false;
            }
            if (modifiers["repeat"].GetBool()) {
                musicPlayTsf.MutableRepeat()->SetRepeatValue("All");
            }
        }
    }

    return true;
}
