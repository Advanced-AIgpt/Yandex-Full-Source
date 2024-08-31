#include "collect_from_context.h"
#include <alice/nlu/granet/lib/sample/entity.h>
#include <alice/nlu/granet/lib/sample/entity_utils.h>
#include <alice/nlu/granet/lib/user_entity/dictionary.h>

#include <alice/protos/data/search_result/search_result.pb.h>
#include <alice/protos/data/video/video.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/video_common/device_helpers.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>

#include <library/cpp/iterator/enumerate.h>

namespace NGranet::NUserEntity {

using namespace NJson;

namespace {

void AddItemToDicts(TStringBuf name, size_t number, bool isVisible, TEntityDict &numDict, TEntityDict &nameDict) {
    const TString value = ToString(number);
    const TString flags = isVisible ? NEntityFlags::GALLERY_VISIBLE_ITEM : TString();
    numDict.Items.push_back({value, flags, value});
    nameDict.Items.push_back({value, flags, TString(name)});
}

static void CollectVideoGalleryItems(const NAlice::TDeviceState& deviceState, TEntityDicts* dicts) {
    Y_ENSURE(dicts);
    NAlice::NVideoCommon::EScreenId currentScreen = NAlice::NVideoCommon::CurrentScreenId(deviceState);
    if (!NAlice::NVideoCommon::IsItemSelectionAvailable(currentScreen)) {
        return;
    }

    const auto& screenState = deviceState.GetVideo().GetScreenState();
    const auto& screenStateVideoItems = screenState.GetRawItems();

    NSc::TValue viewStateJson = NSc::TValue::FromJson(NAlice::JsonStringFromProto(deviceState.GetVideo().GetViewState()));
    const auto& viewStateVideoItems = NAlice::NVideoCommon::GetWebViewGalleryItems(viewStateJson);

    bool searchResultsScreenHasItems = false;
    const auto& tvInterfaceState = deviceState.GetVideo().GetTvInterfaceState();
    if (tvInterfaceState.HasSearchResultsScreen()) {
        const auto& searchResultScreen = tvInterfaceState.GetSearchResultsScreen();
        for (size_t i = 0; i < searchResultScreen.GalleriesSize(); ++i) {
            if (searchResultScreen.GetGalleries(i).ItemsSize() > 0) {
                searchResultsScreenHasItems = true;
                break;
            }
        }
    }

    if (screenStateVideoItems.empty() && viewStateVideoItems.empty() && !searchResultsScreenHasItems) {
        return;
    }

    TEntityDict& numDict = dicts->AddDict(TYPE_GALLERY_ITEM_NUM, 0);
    TEntityDict& nameDict = dicts->AddDict(TYPE_GALLERY_ITEM_NAME, EDF_ENABLE_PARTIAL_MATCH);

    if (currentScreen == NAlice::NVideoCommon::EScreenId::Gallery) { // native video-gallery
        THashSet<size_t> visibleItems(screenState.GetVisibleItems().size());
        for (const auto& item : screenState.GetVisibleItems()) {
            visibleItems.insert(item + 1);
        }

        for (const auto& [index, item] : Enumerate(screenStateVideoItems)) {
            size_t num = index + 1;
            const TString &name = item.GetName();
            if (name.empty()) {
                continue;
            }
            AddItemToDicts(name, num, visibleItems.contains(num), numDict, nameDict);
        }
    } else if (NAlice::NVideoCommon::IsWebViewGalleryScreen(currentScreen)){ // webview video-galleries
        for (const auto& item : viewStateVideoItems) {
            size_t num = item["number"].GetIntNumber();
            if (num == 0) {
                continue;
            }

            const TStringBuf name = item["title"].GetString();
            if (name.empty()) {
                continue;
            }

            AddItemToDicts(name, num, item["active"].GetBool(), numDict, nameDict);
        }
    } else if (currentScreen == NAlice::NVideoCommon::EScreenId::SearchResults && tvInterfaceState.HasSearchResultsScreen()) {
        const auto& searchResultScreen = tvInterfaceState.GetSearchResultsScreen();
        for (size_t i = 0; i < searchResultScreen.GalleriesSize(); ++i) {
            const auto& gallery = searchResultScreen.GetGalleries(i);
            for (size_t j = 0; j < gallery.ItemsSize(); ++j) {
                const auto& item = gallery.GetItems(j);
                size_t num = item.GetNumber();
                if (num == 0) {
                    continue;
                }

                bool isVisible = item.GetVisible();

                TStringBuf name;
                if (item.HasVideoItem()) {
                    name = item.GetVideoItem().GetName();
                } else if (item.HasPersonItem()) {
                    name = item.GetPersonItem().GetName();
                } else if (item.HasCollectionItem()) {
                    name = item.GetCollectionItem().GetTitle();
                }

                if (name.empty()) {
                    continue;
                }

                AddItemToDicts(name, num, isVisible, numDict, nameDict);
            }
        }
    }
}

static void CollectNavigatorFavorites(const NAlice::TDeviceState& deviceState, TEntityDicts* dicts) {
    Y_ENSURE(dicts);
    const auto& favorites = deviceState.GetNavigator().GetUserFavorites();
    if (favorites.empty()) {
        return;
    }

    TEntityDict& dict = dicts->AddDict(TYPE_NAVI_FAVORITES_ITEM, EDF_ENABLE_PARTIAL_MATCH);
    for (const auto& [index, item] : Enumerate(favorites)) {
        size_t num = index + 1;
        const TString& name = item.GetName();
        if (name.empty()) {
            continue;
        }
        dict.Items.push_back({ToString(num), "", name});
    }
}

} // namespace

void CollectDicts(const NAlice::TDeviceState& deviceState, TEntityDicts* dicts) {
    CollectVideoGalleryItems(deviceState, dicts);
    CollectNavigatorFavorites(deviceState, dicts);
}

// For tests only
void CollectDicts(const TJsonValue& deviceState, TEntityDicts* dicts) {
    NAlice::TDeviceState deviceStateProto;
    NAlice::JsonToProto(deviceState, deviceStateProto);
    CollectDicts(deviceStateProto, dicts);
}

NSc::TValue CollectDictsAsTValue(const NAlice::TDeviceState& deviceState) {
    TEntityDicts dicts;
    CollectDicts(deviceState, &dicts);
    return dicts.ToTValue();
}

TString CollectDictsAsBase64(const NAlice::TDeviceState& deviceState) {
    TEntityDicts dicts;
    CollectDicts(deviceState, &dicts);
    return dicts.ToBase64();
}

} // namespace NGranet::NUserEntity
