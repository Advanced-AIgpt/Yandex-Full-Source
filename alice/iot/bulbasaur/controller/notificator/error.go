package notificator

import (
	"a.yandex-team.ru/library/go/core/xerrors"
)

var DeviceOfflineError = xerrors.New("device is offline")
