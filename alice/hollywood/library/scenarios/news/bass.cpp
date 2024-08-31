#include "bass.h"

namespace NAlice::NHollywood {

const NJson::TJsonValue NULL_JSON{NJson::JSON_NULL};

NJson::TJsonValue GetBassResponse(const TNewsState& state) {
    const TString& stateBassResponseString = state.GetBassResponse();
    return stateBassResponseString.is_null() ? NULL_JSON : JsonFromString(stateBassResponseString);
}

const NJson::TJsonValue& GetSlot(const NJson::TJsonValue& bassResponse, const TString name) {
    for (const auto& slot : bassResponse["form"]["slots"].GetArray()) {
        if (slot["name"] == name) {
            return slot;
        }
    }
    return NULL_JSON;
}

const NJson::TJsonValue& GetSlotValue(const NJson::TJsonValue& bassResponse, const TString name) {
    return GetSlot(bassResponse, name)["value"];
}

void AppendBassResponse(NJson::TJsonValue& bassResponse, const NJson::TJsonValue& add) {
    auto& slots = bassResponse["form"]["slots"];
    if (!slots.IsArray()) {
        bassResponse = add;
        return;
    }
    for (auto& slot : slots.GetArraySafe()) {
        if (slot["name"] == "news") {
            auto& news = slot["value"]["news"];
            for(const auto& addNews : GetSlotValue(add, "news")["news"].GetArray()) {
                news.AppendValue(addNews);
            }
            return;
        }
    }
    // slot "news" is not found
    bassResponse = add;
}

const NJson::TJsonValue& GetNewsSlot(const NJson::TJsonValue& bassResponse) {
    const auto& slots = bassResponse["form"]["slots"];
    for (const auto& slot : slots.GetArray()) {
        if (slot["name"] == "news") {
            return slot["value"]["news"];
        }
    }
    return NULL_JSON;
}

int GetNewsCount(const NJson::TJsonValue& newsSlot) {
    if (newsSlot.IsNull()) {
        return 0;
    }
    return newsSlot.GetArray().size();
}

} // namespace NAlice::NHollywood
