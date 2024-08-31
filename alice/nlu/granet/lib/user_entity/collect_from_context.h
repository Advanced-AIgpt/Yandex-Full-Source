#pragma once

#include "dictionary.h"

#include <alice/megamind/protos/common/device_state.pb.h>

namespace NGranet::NUserEntity {

inline const TStringBuf TYPE_GALLERY_ITEM_NUM = "device.gallery_item_num";
inline const TStringBuf TYPE_GALLERY_ITEM_NAME = "device.gallery_item_name";
inline const TStringBuf TYPE_NAVI_FAVORITES_ITEM = "device.navi_favorites_item";

// For tests only
void CollectDicts(const NJson::TJsonValue& deviceState, TEntityDicts* dicts);

NSc::TValue CollectDictsAsTValue(const NAlice::TDeviceState& deviceState);
TString CollectDictsAsBase64(const NAlice::TDeviceState& deviceState);

} // namespace NGranet::NUserEntity
