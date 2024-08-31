package tools

import (
	"strconv"

	"a.yandex-team.ru/rtc/mediator/cityhash"
)

func Huidify(userID uint64) uint64 {
	return cityhash.Hash64([]byte(strconv.FormatUint(userID, 10)))
}

func HuidifyString(value string) uint64 {
	return cityhash.Hash64([]byte(value))
}
