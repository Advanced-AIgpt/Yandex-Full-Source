package megamind

import (
	"sort"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/stretchr/testify/assert"
)

func TestDeviceCapabilityQueryResultsSorting(t *testing.T) {
	actualResults := DeviceCapabilityQueryResults{
		{Name: "Лампочка 2", DeviceType: model.LightDeviceType},
		{Name: "Лампочка 1", DeviceType: model.LightDeviceType},
		{Name: "Xiaomi fan 2", DeviceType: model.FanDeviceType},
		{Name: "Xiaomi fan 1", DeviceType: model.FanDeviceType},
		{Name: "Zhimi fan", DeviceType: model.FanDeviceType},
		{Name: "Лампа", DeviceType: model.LightDeviceType},
		{Name: "Белая лента", DeviceType: model.LightDeviceType},
	}
	sort.Sort(DeviceCapabilityQueryResultsSorting(actualResults))
	expectedResults := DeviceCapabilityQueryResults{
		{Name: "Xiaomi fan 1", DeviceType: model.FanDeviceType},
		{Name: "Xiaomi fan 2", DeviceType: model.FanDeviceType},
		{Name: "Zhimi fan", DeviceType: model.FanDeviceType},
		{Name: "Белая лента", DeviceType: model.LightDeviceType},
		{Name: "Лампа", DeviceType: model.LightDeviceType},
		{Name: "Лампочка 1", DeviceType: model.LightDeviceType},
		{Name: "Лампочка 2", DeviceType: model.LightDeviceType},
	}
	assert.Equal(t, expectedResults, actualResults)

	expectedLights := DeviceCapabilityQueryResults{
		{Name: "Белая лента", DeviceType: model.LightDeviceType},
		{Name: "Лампа", DeviceType: model.LightDeviceType},
		{Name: "Лампочка 1", DeviceType: model.LightDeviceType},
		{Name: "Лампочка 2", DeviceType: model.LightDeviceType},
	}
	expectedFans := DeviceCapabilityQueryResults{
		{Name: "Xiaomi fan 1", DeviceType: model.FanDeviceType},
		{Name: "Xiaomi fan 2", DeviceType: model.FanDeviceType},
		{Name: "Zhimi fan", DeviceType: model.FanDeviceType},
	}
	// check that after grouping order stays the same
	assert.Equal(t, expectedLights, actualResults.GroupByDeviceType()[model.LightDeviceType])
	assert.Equal(t, expectedFans, actualResults.GroupByDeviceType()[model.FanDeviceType])

}

func TestDevicePropertyQueryResultsSorting(t *testing.T) {
	actualResults := DevicePropertyQueryResults{
		{Name: "Лампочка 2", DeviceType: model.LightDeviceType},
		{Name: "Лампочка 1", DeviceType: model.LightDeviceType},
		{Name: "Xiaomi fan 2", DeviceType: model.FanDeviceType},
		{Name: "Xiaomi fan 1", DeviceType: model.FanDeviceType},
		{Name: "Zhimi fan", DeviceType: model.FanDeviceType},
		{Name: "Лампа", DeviceType: model.LightDeviceType},
		{Name: "Белая лента", DeviceType: model.LightDeviceType},
	}
	sort.Sort(DevicePropertyQueryResultsSorting(actualResults))
	expectedResults := DevicePropertyQueryResults{
		{Name: "Xiaomi fan 1", DeviceType: model.FanDeviceType},
		{Name: "Xiaomi fan 2", DeviceType: model.FanDeviceType},
		{Name: "Zhimi fan", DeviceType: model.FanDeviceType},
		{Name: "Белая лента", DeviceType: model.LightDeviceType},
		{Name: "Лампа", DeviceType: model.LightDeviceType},
		{Name: "Лампочка 1", DeviceType: model.LightDeviceType},
		{Name: "Лампочка 2", DeviceType: model.LightDeviceType},
	}
	assert.Equal(t, expectedResults, actualResults)

	expectedLights := DevicePropertyQueryResults{
		{Name: "Белая лента", DeviceType: model.LightDeviceType},
		{Name: "Лампа", DeviceType: model.LightDeviceType},
		{Name: "Лампочка 1", DeviceType: model.LightDeviceType},
		{Name: "Лампочка 2", DeviceType: model.LightDeviceType},
	}
	expectedFans := DevicePropertyQueryResults{
		{Name: "Xiaomi fan 1", DeviceType: model.FanDeviceType},
		{Name: "Xiaomi fan 2", DeviceType: model.FanDeviceType},
		{Name: "Zhimi fan", DeviceType: model.FanDeviceType},
	}
	// check that after grouping order stays the same
	assert.Equal(t, expectedLights, actualResults.GroupByDeviceType()[model.LightDeviceType])
	assert.Equal(t, expectedFans, actualResults.GroupByDeviceType()[model.FanDeviceType])
}
