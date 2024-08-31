package callbacks

import (
	"context"
	"encoding/json"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/library/go/logbroker"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/rtc/mediator/cityhash"
)

type Controller struct {
	logger         log.Logger
	writePool      logbroker.WritePool
	partitionCount uint32
}

func NewController(
	logger log.Logger,
	writePool logbroker.WritePool,
	partitionCount uint32,
) IController {
	return &Controller{
		logger:         logger,
		writePool:      writePool,
		partitionCount: partitionCount,
	}
}

func (c *Controller) SendCallback(ctx context.Context, skillID string, stateCallback callback.UpdateStateRequest) error {
	if skillID == "" {
		return xerrors.Errorf("skillID can't be empty")
	}

	if stateCallback.Payload == nil {
		return xerrors.Errorf("callback payload can't be nil")
	}

	message := callback.SkillUpdateStateRequest{ // wrap message with info about
		SkillID:  skillID,
		Callback: stateCallback,
	}

	rawMessage, err := json.Marshal(message)
	if err != nil {
		return xerrors.Errorf("failed to marshal message to json: %w", err)
	}

	partition := selectPartition(stateCallback.Payload.UserID, c.partitionCount)
	// write message to logbroker
	return c.writePool.WriteWithAck(ctx, partition, rawMessage)
}

func selectPartition(shardKey string, partitionsCount uint32) uint32 {
	hash := cityhash.Hash64([]byte(shardKey))
	partition := hash % uint64(partitionsCount)
	return uint32(partition)
}
