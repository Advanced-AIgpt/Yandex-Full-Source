package mobile

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"github.com/stretchr/testify/assert"
)

func TestDeviceTypesView_FromDevice_Switch(t *testing.T) {
	device := model.Device{
		Type:         model.SwitchDeviceType,
		OriginalType: model.SwitchDeviceType,
		Groups:       make([]model.Group, 0),
	}

	expectedView := DeviceTypesView{
		Types: []DeviceTypeView{
			{
				ID:     model.LightDeviceType,
				Name:   LightTypeName,
				Active: false,
			},
			{
				ID:     model.SwitchDeviceType,
				Name:   SwitchTypeName,
				Active: true,
			},
		},
	}

	deviceTypesView := DeviceTypesView{}
	deviceTypesView.FromDevice(device)

	assert.Equal(t, expectedView, deviceTypesView)
}

func TestDeviceTypesView_FromDevice_Socket(t *testing.T) {
	device := model.Device{
		Type:         model.SocketDeviceType,
		OriginalType: model.SocketDeviceType,
		Groups:       make([]model.Group, 0),
	}

	expectedView := DeviceTypesView{
		Types: []DeviceTypeView{
			{
				ID:     model.LightDeviceType,
				Name:   LightTypeName,
				Active: false,
			},
			{
				ID:     model.SocketDeviceType,
				Name:   SocketTypeName,
				Active: true,
			},
		},
	}

	deviceTypesView := DeviceTypesView{}
	deviceTypesView.FromDevice(device)

	assert.Equal(t, expectedView, deviceTypesView)
}

func TestDeviceTypesView_FromDevice_Socket_Light(t *testing.T) {
	device := model.Device{
		Type:         model.LightDeviceType,
		OriginalType: model.SocketDeviceType,
		Groups:       make([]model.Group, 0),
	}

	expectedView := DeviceTypesView{
		Types: []DeviceTypeView{
			{
				ID:     model.LightDeviceType,
				Name:   LightTypeName,
				Active: true,
			},
			{
				ID:     model.SocketDeviceType,
				Name:   SocketTypeName,
				Active: false,
			},
		},
	}

	deviceTypesView := DeviceTypesView{}
	deviceTypesView.FromDevice(device)

	assert.Equal(t, expectedView, deviceTypesView)
}

func TestDeviceTypesView_FromDevice_Light(t *testing.T) {
	device := model.Device{
		Type:         model.LightDeviceType,
		OriginalType: model.LightDeviceType,
		Groups:       make([]model.Group, 0),
	}

	expectedView := DeviceTypesView{
		Types: []DeviceTypeView{},
	}

	deviceTypesView := DeviceTypesView{}
	deviceTypesView.FromDevice(device)

	assert.Equal(t, expectedView, deviceTypesView)
}

func TestDeviceTypesView_FromDevice_Switch_InGroups(t *testing.T) {
	device := model.Device{
		Type:         model.SwitchDeviceType,
		OriginalType: model.SwitchDeviceType,
		Groups: []model.Group{
			{
				ID: "g-1",
			},
		},
	}

	expectedView := DeviceTypesView{
		Types: []DeviceTypeView{
			{
				ID:     model.LightDeviceType,
				Name:   LightTypeName,
				Active: false,
			},
			{
				ID:     model.SwitchDeviceType,
				Name:   SwitchTypeName,
				Active: true,
			},
		},
		TypeSwitchError: tools.AOS(model.DeviceInGroupTypeSwitchError),
	}

	deviceTypesView := DeviceTypesView{}
	deviceTypesView.FromDevice(device)

	assert.Equal(t, expectedView, deviceTypesView)
}
