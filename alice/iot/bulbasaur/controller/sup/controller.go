package sup

import (
	"context"
	"fmt"
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
	Client sup.IClient
	Logger log.Logger
}

func (c *Controller) SendPushToUser(ctx context.Context, user model.User, pushInfo PushInfo) error {
	return c.sendPushToUsers(ctx, []model.User{user}, pushInfo)
}

func (c *Controller) sendPushToUsers(ctx context.Context, users model.Users, pushInfo PushInfo) error {
	receivers := make([]sup.IReceiver, 0, len(users))
	for _, user := range users {
		receivers = append(receivers, sup.UIDReceiver{
			UID: strconv.FormatUint(user.ID, 10),
		})
	}

	// IOT-1427: iotApp use different frontend package
	iotAppLink := strings.Replace(pushInfo.Link, "https://yandex.ru/quasar", "https://yandex.ru/iot", 1)

	pushRequest := sup.PushRequest{
		Receivers: receivers,
		TTL:       defaultSecondsTTL, // 3600 seconds ttl
		Notification: sup.PushNotification{
			Title:  IOTDefaultTitle,
			Icon:   AliceLogoIcon,
			IconID: AliceLogoIconID,
			Body:   pushInfo.Text,
			Link:   SearchAppLink(pushInfo.Link),
		},
		Project: IOTProjectName,
		Data: sup.PushData{
			PushID: pushInfo.ID,
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
						Body:  pushInfo.Text,
						Link:  IotAppAndroidLink(iotAppLink),
					},
					Data: &sup.PushData{
						PushURI:    url.QueryEscape(iotAppLink),
						PushAction: sup.URIPushActionType,
						Tag:        fmt.Sprintf("%s.%s", pushInfo.ID, pushInfo.Tag),
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
						Body:  pushInfo.Text,
						Link:  IotAppIOSLink(iotAppLink),
					},
					Data: &sup.PushData{
						Tag: fmt.Sprintf("%s.%s", pushInfo.ID, pushInfo.Tag),
					},
				},
			},
		},
		ThrottlePolicies: nil,
	}

	if pushInfo.ThrottlePolicyID != "" {
		pushRequest.ThrottlePolicies = &sup.ThrottlePolicies{
			InstallID: pushInfo.ThrottlePolicyID,
			DeviceID:  pushInfo.ThrottlePolicyID,
		}
	}

	pushResponse, err := c.Client.SendPush(ctx, pushRequest)
	if err != nil {
		return xerrors.Errorf("failed to send push to users %v: %w", users.IDs(), err)
	}
	ctxlog.Infof(ctx, c.Logger, "sent push to sup, push id: %s, receivers num: %d", pushResponse.ID, pushResponse.Receivers)
	return nil
}
