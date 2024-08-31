package push

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

type DiscoveryRequest struct {
	Timestamp timestamp.PastTimestamp  `json:"ts"`
	Payload   adapter.DiscoveryPayload `json:"payload"`
	YandexUID *uint64                  `json:"yandex_uid,omitempty"`
}

func (dr DiscoveryRequest) ToAdapterDiscoveryResult() adapter.DiscoveryResult {
	return adapter.DiscoveryResult{
		Timestamp: dr.Timestamp,
		Payload:   dr.Payload,
	}
}
