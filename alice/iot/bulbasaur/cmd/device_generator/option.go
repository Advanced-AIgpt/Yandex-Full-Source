package main

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type Option interface {
	isOption()
}

//  on off options
type unretrievableOnOffOption struct{}

func (unretrievableOnOffOption) isOption() {}

func WithUnretrievableOnOff() Option {
	return unretrievableOnOffOption{}
}

type splitOnOffOption struct{ Split bool }

func (splitOnOffOption) isOption() {}

func WithSplitOnOffOption(split bool) Option {
	return splitOnOffOption{Split: split}
}

// mode options

// thermostat
type unretrievableThermostatOption struct{}

func (unretrievableThermostatOption) isOption() {}

func WithUnretrievableThermostat() Option {
	return unretrievableThermostatOption{}
}

type thermostatModeOption struct{ Modes []model.Mode }

func (thermostatModeOption) isOption() {}

func WithThermostatModes(modes ...model.ModeValue) Option {
	modelModes := make([]model.Mode, 0, len(modes))
	for _, modeValue := range modes {
		modelModes = append(modelModes, model.KnownModes[modeValue])
	}
	return thermostatModeOption{Modes: modelModes}
}

// fanspeed
type unretrievableFanSpeedOption struct{}

func (unretrievableFanSpeedOption) isOption() {}

func WithUnretrievableFanSpeed() Option {
	return unretrievableFanSpeedOption{}
}

type fanSpeedModeOption struct{ Modes []model.Mode }

func (fanSpeedModeOption) isOption() {}

func WithFanSpeedModes(modes ...model.ModeValue) Option {
	modelModes := make([]model.Mode, 0, len(modes))
	for _, modeValue := range modes {
		modelModes = append(modelModes, model.KnownModes[modeValue])
	}
	return fanSpeedModeOption{Modes: modelModes}
}

// input_source
type unretrievableInputSourceOption struct{}

func (unretrievableInputSourceOption) isOption() {}

func WithUnretrievableInputSource() Option {
	return unretrievableInputSourceOption{}
}

type inputSourceModeOption struct{ Modes []model.Mode }

func (inputSourceModeOption) isOption() {}

func WithInputSourceModes(modes ...model.ModeValue) Option {
	modelModes := make([]model.Mode, 0, len(modes))
	for _, modeValue := range modes {
		modelModes = append(modelModes, model.KnownModes[modeValue])
	}
	return inputSourceModeOption{Modes: modelModes}
}

type teaModesOption struct{ teaModes []model.Mode }

func (teaModesOption) isOption() {}

func WithTeaModes() Option {
	return teaModesOption{teaModes: []model.Mode{
		model.KnownModes[model.BlackTeaMode],
		model.KnownModes[model.GreenTeaMode],
		model.KnownModes[model.PuerhTeaMode],
		model.KnownModes[model.WhiteTeaMode],
		model.KnownModes[model.OolongTeaMode],
		model.KnownModes[model.RedTeaMode],
		model.KnownModes[model.HerbalTeaMode],
		model.KnownModes[model.FlowerTeaMode],
	}}
}

// range

// temperature
type unretrievableTemperatureRangeOption struct{}

func (unretrievableTemperatureRangeOption) isOption() {}

func WithUnretrievableTemperatureRange() Option {
	return unretrievableTemperatureRangeOption{}
}

type noTemperatureRangeOption struct{}

func (noTemperatureRangeOption) isOption() {}

func WithNoTemperatureRangeOption() Option {
	return noTemperatureRangeOption{}
}

type temperatureRangeBoundsOption struct{ min, max float64 }

func (temperatureRangeBoundsOption) isOption() {}

func WithTemperatureRangeBounds(min, max float64) Option {
	return temperatureRangeBoundsOption{min: min, max: max}
}

// channel
type unretrievableChannelRangeOption struct{}

func (unretrievableChannelRangeOption) isOption() {}

func WithUnretrievableChannelRange() Option {
	return unretrievableChannelRangeOption{}
}

type channelRangeBoundsOption struct{ min, max float64 }

func (channelRangeBoundsOption) isOption() {}

func WithChannelRangeBounds(min, max float64) Option {
	return channelRangeBoundsOption{min: min, max: max}
}

// volume
type unretrievableVolumeRangeOption struct{}

func (unretrievableVolumeRangeOption) isOption() {}

func WithUnretrievableVolumeRange() Option {
	return unretrievableVolumeRangeOption{}
}

// toggle
// pause
type unretrievablePauseToggleOption struct{}

func (unretrievablePauseToggleOption) isOption() {}

func WithUnretrievablePauseToggle() Option {
	return unretrievablePauseToggleOption{}
}

// mute
type unretrievableMuteToggleOption struct{}

func (unretrievableMuteToggleOption) isOption() {}

func WithUnretrievableMuteToggle() Option {
	return unretrievableMuteToggleOption{}
}

// custom button

type customButtonOption struct{ name string }

func (customButtonOption) isOption() {}

func WithCustomButton(name string) Option {
	return customButtonOption{name: name}
}

// color_setting

type noColorModelOption struct{}

func (noColorModelOption) isOption() {}

func WithNoColorModel() Option {
	return noColorModelOption{}
}

// properties

// humidity

type unreportableHumidityPropertyOption struct{}

func (unreportableHumidityPropertyOption) isOption() {}

func WithUnreportableHumidityProperty() Option {
	return unreportableHumidityPropertyOption{}
}

// temperature

type unreportableTemperaturePropertyOption struct{}

func (unreportableTemperaturePropertyOption) isOption() {}

func WithUnreportableTemperatureProperty() Option {
	return unreportableTemperaturePropertyOption{}
}
