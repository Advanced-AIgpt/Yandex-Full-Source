package provider

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/library/go/core/metrics"
)

type IActionSignals interface {
	GetRequestSignals() RequestSignals
	GetSuccess() metrics.Counter
	GetTotalRequests() metrics.Counter
	RecordErrors(errors adapter.ErrorCodeCountMap) int64
}

type IQuerySignals interface {
	GetRequestSignals() RequestSignals
	GetSuccess() metrics.Counter
	GetTotalRequests() metrics.Counter
	RecordErrors(errors adapter.ErrorCodeCountMap) int64
}
