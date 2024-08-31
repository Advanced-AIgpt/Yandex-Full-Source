package sup

import (
	"context"
	"math/rand"
	"net/url"
	"strconv"
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/sup"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Controller struct {
	Logger log.Logger
	Sup    sup.IClient
}

func (c *Controller) WifiIs5GhzPush(ctx context.Context, userID uint64, deviceType model.DeviceType) error {
	texts := wifiIs5GhzPushTexts[deviceType]
	return c.simplePushRequest(ctx, userID, voiceDiscoveryWifiIs5GhzStatID, texts[rand.Intn(len(texts))], GetDiscoveryLink(deviceType))
}

func (c *Controller) DiscoveryErrorPush(ctx context.Context, userID uint64, deviceType model.DeviceType) error {
	texts := discoveryErrorPushTexts[deviceType]
	return c.simplePushRequest(ctx, userID, voiceDiscoveryFailureStatID, texts[rand.Intn(len(texts))], GetDiscoveryLink(deviceType))
}

func (c *Controller) DiscoveryNotAllowedSpeakerPush(ctx context.Context, userID uint64, deviceType model.DeviceType) error {
	texts := discoveryNotAllowedPushTexts[deviceType]
	return c.simplePushRequest(ctx, userID, voiceDiscoveryNotAllowedSpeakerStatID, texts[rand.Intn(len(texts))], GetDiscoveryLink(deviceType))
}

func (c *Controller) DiscoveryNotAllowedClientPush(ctx context.Context, userID uint64, deviceType model.DeviceType) error {
	texts := discoveryNotAllowedPushTexts[deviceType]
	return c.simplePushRequest(ctx, userID, voiceDiscoveryNotAllowedClientStatID, texts[rand.Intn(len(texts))], GetDiscoveryLink(deviceType))
}

func (c *Controller) simplePushRequest(ctx context.Context, userID uint64, statID string, bodyText string, link string) error {
	receivers := make([]sup.IReceiver, 0)
	receivers = append(receivers, sup.UIDReceiver{
		UID: strconv.FormatUint(userID, 10),
	})

	// IOT-1427: iotApp use different frontend package
	iotAppLink := strings.Replace(link, "https://yandex.ru/quasar", "https://yandex.ru/iot", 1)

	pushRequest := sup.PushRequest{
		Receivers: receivers,
		TTL:       defaultSecondsTTL,
		Notification: sup.PushNotification{
			Title:  IOTDefaultTitle,
			Body:   bodyText,
			Icon:   AliceLogoIcon,
			IconID: AliceLogoIconID,
			Link:   sup.SearchAppLinkWrapper(sup.QuasarLinkWrapper(link)),
		},
		Repacks: sup.Repacks{
			sup.Repack{
				App: sup.RepackApp{
					Platform: "android",
					Apps:     []string{"com.yandex.iot", "com.yandex.iot.inhouse", "com.yandex.iot.dev"},
					Version:  "21.111",
				},
				Push: sup.RepackPush{
					Notification: sup.PushNotification{
						Title: IOTAppDefaultTitle,
						Body:  bodyText,
						Link:  sup.IoTAndroidLinkWrapper(iotAppLink),
					},
					Data: &sup.PushData{
						PushURI:    url.QueryEscape(iotAppLink),
						PushAction: sup.URIPushActionType,
					},
				},
			},
			sup.Repack{
				App: sup.RepackApp{
					Platform: "ios",
					Apps:     []string{"com.yandex.iot", "com.yandex.iot.inhouse", "com.yandex.iot.dev"},
					Version:  "85.00",
				},
				Push: sup.RepackPush{
					Notification: sup.PushNotification{
						Title: IOTAppDefaultTitle,
						Body:  bodyText,
						Link:  sup.IoTIOSLinkWrapper(iotAppLink),
					},
				},
			},
		},
		Project: IOTSupProjectName,
		Data: sup.PushData{
			PushID: voiceDiscoveryErrorPushID,
			StatID: statID,
		},
		ThrottlePolicies: &sup.ThrottlePolicies{
			InstallID: voiceDiscoveryThrottlePolicy,
			DeviceID:  voiceDiscoveryThrottlePolicy,
		},
	}

	response, err := c.Sup.SendPush(ctx, pushRequest)
	if err != nil {
		return xerrors.Errorf("failed to send push to user %d: %w", userID, err)
	}
	ctxlog.Infof(ctx, c.Logger, "sent push to sup, push id: %s, receivers: %d", response.ID, response.Receivers)
	return nil
}

func GetDiscoveryLink(deviceType model.DeviceType) string {
	return discoveryLinks[deviceType]
}
