#include "antirobot.h"

#include <util/stream/str.h>


namespace NAlice::NCuttlefish::NAppHostServices {

class TRequestBuilder {
public:
    TRequestBuilder() {
        Stream << "POST /alice HTTP/1.1\r\n";
    }

    template <typename T>
    TRequestBuilder& AddHeader(TStringBuf name, const T& value) {
        Stream << name << ": " << value << "\r\n";
        return *this;
    }

    TRequestBuilder& AddBody(TStringBuf value) {
        AddHeader("Content-Length", value.size());
        Stream << "\r\n" << value;
        return *this;
    }

    TString ToString() const {
        return Stream.Str();
    }

private:
    TStringStream Stream;
};


TMaybe<NAppHostHttp::THttpRequest> TAntirobotClient::CreateRequest(
    const NAliceProtocol::TSessionContext& ctx,
    const NAliceProtocol::TAntirobotInputSettings& settings,
    const NAliceProtocol::TAntirobotInputData& data
) {
    if (settings.GetMode() == NAliceProtocol::EAntirobotMode::OFF) {
        return Nothing();
    }

    TRequestBuilder b;

    // fixed Host for antirobot
    b.AddHeader("Host", "uniproxy.alice.yandex.net");

    if (ctx.HasSessionId()) {
        b.AddHeader("X-Alice-Session-Id", ctx.GetSessionId());
    }

    if (ctx.HasSurface()) {
        b.AddHeader("X-Alice-Surface", ctx.GetSurface());
    }

    if (ctx.HasAppId()) {
        b.AddHeader("X-Alice-AppId", ctx.GetAppId());
    }

    if (ctx.HasSpeechkitVersion()) {
        b.AddHeader("X-Alice-Speechkit-Version", ctx.GetSpeechkitVersion());
    }

    if (ctx.HasAppVersion()) {
        b.AddHeader("X-Alice-App-Version", ctx.GetAppVersion());
    }

    if (ctx.HasUserInfo()) {
        const auto& info = ctx.GetUserInfo();
        if (info.HasUuid()) {
            b.AddHeader("X-Alice-Uuid", info.GetUuid());
        }

        if (info.HasVinsApplicationUuid()) {
            b.AddHeader("X-Alice-Vins-Uuid", info.GetVinsApplicationUuid());
        }
    }

    if (ctx.HasConnectionInfo()) {
        const auto& info = ctx.GetConnectionInfo();

        if (info.HasOrigin()) {
            b.AddHeader("Origin", info.GetOrigin());
        }

        if (info.HasUserAgent()) {
            b.AddHeader("User-Agent", info.GetUserAgent());
        }
    }

    if (ctx.HasDeviceInfo()) {
        const auto& info = ctx.GetDeviceInfo();
        if (info.HasDevice()) {
            b.AddHeader("X-Alice-Device", info.GetDevice());
        }

        if (info.HasDeviceManufacturer()) {
            b.AddHeader("X-Alice-Device-Manufacturer", info.GetDeviceManufacturer());
        }

        if (info.HasDeviceModel()) {
            b.AddHeader("X-Alice-Device-Model", info.GetDeviceModel());
        }

        if (info.HasDeviceModification()) {
            b.AddHeader("X-Alice-Device-Modification", info.GetDeviceModification());
        }

        if (info.HasPlatform()) {
            b.AddHeader("X-Alice-Platform", info.GetPlatform());
        }

        if (info.SupportedFeaturesSize() > 0) {
            b.AddHeader("X-Alice-Has-Supported-Features", "true");
        }

        if (info.WifiNetworksSize() > 0) {
            b.AddHeader("X-Alice-Has-Wifi-Networks", "true");
        }
    }


    if (data.HasBody()) {
        b.AddBody(data.GetBody());
    }

    NAppHostHttp::THttpRequest req;

    req.SetPath("/fullreq");
    req.SetMethod(NAppHostHttp::THttpRequest::Post);

    if (data.HasForwardedFor()) {
        auto *header = req.AddHeaders();
        header->SetName("X-Forwarded-For-Y");
        header->SetValue(data.GetForwardedFor());
    }

    if (data.HasJa3()) {
        auto *header = req.AddHeaders();
        header->SetName("X-Yandex-Ja3");
        header->SetValue(data.GetJa3());
    }

    if (data.HasJa4()) {
        auto *header = req.AddHeaders();
        header->SetName("X-Yandex-Ja4");
        header->SetValue(data.GetJa4());
    }

    req.SetContent(b.ToString());

    return req;
}


bool TAntirobotClient::ParseResponseTo(const NAppHostHttp::THttpResponse& httpResponse, NAliceProtocol::TRobotnessData* out) {
    if (!out) return false;

    switch (httpResponse.GetStatusCode()) {
        case 200:
            out->SetIsRobot(false);
            out->SetRobotness(0.0); // TODO: read this value from header when we switch to blocking mode instead of captcha-mode
            break;
        case 302:
        case 403:
            out->SetIsRobot(true);
            out->SetRobotness(1.0); // TODO: read this value from header when we switch to blocking mode instead of captcha-mode
            break;
        default:
            out->SetIsRobot(false);
            out->SetRobotness(0.0); // TODO: read this value from header when we switch to blocking mode instead of captcha-mode
            break;
    }

    return true;
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
