package bass

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type IBass interface {
	SendPush(ctx context.Context, userID uint64, deviceid string, data string, actionInstance model.QuasarServerActionCapabilityInstance) error
	SendSemanticFramePush(ctx context.Context, userID uint64, deviceID string, semanticFrame ITypedSemanticFrame, analyticsData SemanticFrameAnalyticsData) error
}

type ITypedSemanticFrame interface {
	Type() SemanticFrameType
	MarshalJSON() ([]byte, error)
}
