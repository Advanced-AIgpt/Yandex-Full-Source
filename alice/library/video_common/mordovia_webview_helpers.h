#pragma once

#include <alice/library/json/json.h>
#include <alice/library/video_common/defs.h>

#include <alice/megamind/protos/common/device_state.pb.h>

#include <google/protobuf/struct.pb.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>

namespace NAlice::NVideoCommon {

TStringBuf GetWebViewScreenName(const NSc::TValue& viewState);
TStringBuf GetWebViewScreenName(const google::protobuf::Struct& viewState);

const NSc::TArray& GetWebViewGalleryItems(const NSc::TValue& viewState);
const google::protobuf::ListValue& GetWebViewGalleryItems(const google::protobuf::Struct& viewState);
const google::protobuf::Struct& GetWebViewCurrentVideoItem(const google::protobuf::Struct& viewState);
const google::protobuf::Struct& GetWebViewCurrentTvShowItem(const google::protobuf::Struct& viewState);

const TMap<size_t, NSc::TValue> GetVisibleGalleryItems(const NSc::TValue& viewState);
NSc::TValue GetRawVideoGallery(const NSc::TValue& viewState);
NSc::TValue GetRawCurrentVideoItem(const NSc::TValue& viewState);
NSc::TValue GetRawCurrentTvShowItem(const NSc::TValue& viewState);

i64 GetCurrentTvShowSeasonNumber(const NSc::TValue& viewState);

TStringBuf GetUserSubscriptionType(const NSc::TValue& viewState);

class TMordoviaJsCallbackPayload {
public:
    TMordoviaJsCallbackPayload(const google::protobuf::Struct& callbackPayload)
        : CallbackPayload(callbackPayload)
    {}

    TString Command() {
        if (!IsIn(CallbackPayload.fields(), "command")) {
            return TString();
        }
        return CallbackPayload.fields().at("command").string_value();
    }

    NSc::TValue Payload() {
        if (!IsIn(CallbackPayload.fields(), "payload")) {
            return {};
        }
        return NSc::TValue::FromJson(JsonStringFromProto(CallbackPayload.fields().at("payload")));
    }

private:
    google::protobuf::Struct CallbackPayload;
};

TStringBuf GetCurrentViewKey(const TDeviceState& deviceState);
TStringBuf GetCurrentViewKey(const NSc::TValue& deviceState);

bool IsWebViewMainViewKey(TStringBuf viewKey);
bool IsWebViewVideoViewKey(TStringBuf viewKey);

NSc::TValue BuildMordoviaShowPayload(TStringBuf viewKey, const TString& url, const TString& splashDiv, bool doGoBack = false);
NJson::TJsonValue BuildMordoviaShowJsonPayload(TStringBuf viewKey, const TString& url, const TString& splashDiv, bool doGoBack = false);

NSc::TValue BuildChangePathCommandPayload(TStringBuf viewKey, const TString& path, bool clearNavigationHistory = false);
NJson::TJsonValue BuildChangePathCommandJsonPayload(TStringBuf viewKey, const TString& path, bool clearNavigationHistory = false);
TStringBuf GetTestidsFromMegamindCookies(const TStringBuf megamindCookies);
TStringBuf GetTestidsFromMegamindCookies(const ::google::protobuf::Struct& megamindCookies);

} // namespace NAlice::NVideoCommon
