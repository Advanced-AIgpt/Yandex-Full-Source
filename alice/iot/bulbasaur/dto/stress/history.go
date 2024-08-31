package stress

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

// UpdateSolomonStateRequest is a dto for solomon history stress test
// ToDo: delete after stress testing
type UpdateSolomonStateRequest struct {
	Timestamp timestamp.PastTimestamp `json:"timestamp"`
	DeviceID  string                  `json:"device_id"`
	Instance  model.PropertyInstance  `json:"instance"`
	Value     float64                 `json:"value"`
}
