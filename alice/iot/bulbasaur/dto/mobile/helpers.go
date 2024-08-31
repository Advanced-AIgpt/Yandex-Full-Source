package mobile

import (
	"time"

	"a.yandex-team.ru/alice/library/go/timestamp"
)

func formatTimestamp(ts timestamp.PastTimestamp) string {
	return ts.AsTime().UTC().Format(time.RFC3339)
}
