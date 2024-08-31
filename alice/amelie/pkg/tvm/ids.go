package ids

import "a.yandex-team.ru/library/go/yandex/tvm"

const (
	AlicePush = "alice_push"
)

var Services = map[string]tvm.ClientID{
	AlicePush: 2034422,
}

func GetID(name string) uint32 {
	return uint32(Services[name])
}
