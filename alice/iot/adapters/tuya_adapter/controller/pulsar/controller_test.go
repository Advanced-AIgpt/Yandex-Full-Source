package pulsar

import (
	"testing"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/library/go/core/xerrors"
	"github.com/stretchr/testify/assert"
)

func TestController_GetPulsarStatus(t *testing.T) {
	t.Run("noValuableDataError", func(t *testing.T) {
		c := Controller{
			device: &tuya.UserDevice{Category: string(tuya.TuyaLightDeviceType), ProductID: tuya.LampYandexProductID},
		}
		_, _, err := c.GetPulsarStatus(tuya.PulsarStatuses{{Code: "non-existent-trash"}})
		assert.True(t, xerrors.Is(err, NoValuableDataErr))
	})
}
