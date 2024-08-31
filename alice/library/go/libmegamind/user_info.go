package libmegamind

import (
	"a.yandex-team.ru/alice/megamind/protos/blackbox"
)

type UserInfo struct {
	UID     string
	IsStaff bool
}

func NewUserInfo(ui *blackbox.TBlackBoxUserInfo) UserInfo {
	if ui == nil {
		return UserInfo{}
	}

	return UserInfo{
		UID:     ui.Uid,
		IsStaff: ui.IsStaff,
	}
}
