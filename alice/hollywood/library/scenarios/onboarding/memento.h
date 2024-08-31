#pragma once

#include <alice/hollywood/library/framework/framework.h>

#include <dj/services/alisa_skills/server/proto/data/data_types.pb.h>

namespace NAlice::NHollywoodFw::NOnboarding {

using TProtoItems = NProtoBuf::RepeatedPtrField<NDJ::NAS::TProtoItem>;

void UpdateTagStats(const TRunRequest& request, TStorage& storage, const TProtoItems& items);

void AddLastViews(const TRunRequest& request, TStorage& storage, const TProtoItems& items, const ui32 itemType);

} // namespace NAlice::NHollywoodFw::NOnboarding
