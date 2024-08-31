package discovery

import (
	"a.yandex-team.ru/alice/library/go/libmegamind"
)

const (
	CancelDiscoveryCallbackName libmegamind.CallbackName = "cancel_discovery_callback"
)

type CancelCallback struct{}

func (c *CancelCallback) ToCallbackFrameAction() libmegamind.CallbackFrameAction {
	return libmegamind.CallbackFrameAction{
		FrameSlug:    "cancel_discovery",
		FrameName:    "alice.iot.discovery.cancel_search_hints",
		CallbackName: CancelDiscoveryCallbackName,
	}
}
