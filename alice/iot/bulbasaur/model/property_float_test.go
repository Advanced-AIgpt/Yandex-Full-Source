package model

import (
	"fmt"
	"math"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

func TestFloatPropertyState_Validate(t *testing.T) {
	testCases := []struct {
		name       string
		state      IPropertyState
		expectErrs valid.Errors
	}{
		//valid
		{
			name: "valid_water_level_state_100",
			state: FloatPropertyState{
				Instance: WaterLevelPropertyInstance,
				Value:    100,
			},
			expectErrs: nil,
		},
		{
			name: "valid_water_level_state_50",
			state: FloatPropertyState{
				Instance: WaterLevelPropertyInstance,
				Value:    50,
			},
			expectErrs: nil,
		},
		{
			name: "valid_water_level_state_0",
			state: FloatPropertyState{
				Instance: WaterLevelPropertyInstance,
				Value:    0,
			},
			expectErrs: nil,
		},
		{
			name: "valid_humidity_state_100",
			state: FloatPropertyState{
				Instance: HumidityPropertyInstance,
				Value:    100,
			},
			expectErrs: nil,
		},
		{
			name: "valid_humidity_state_50",
			state: FloatPropertyState{
				Instance: HumidityPropertyInstance,
				Value:    50,
			},
			expectErrs: nil,
		},
		{
			name: "valid_humidity_state_0",
			state: FloatPropertyState{
				Instance: HumidityPropertyInstance,
				Value:    0,
			},
			expectErrs: nil,
		},
		{
			name: "valid_co2_level_state_50",
			state: FloatPropertyState{
				Instance: CO2LevelPropertyInstance,
				Value:    50,
			},
			expectErrs: nil,
		},
		{
			name: "valid_co2_level_state_0",
			state: FloatPropertyState{
				Instance: CO2LevelPropertyInstance,
				Value:    0,
			},
			expectErrs: nil,
		},
		{
			name: "valid_amperage_state_50",
			state: FloatPropertyState{
				Instance: AmperagePropertyInstance,
				Value:    50,
			},
			expectErrs: nil,
		},
		{
			name: "valid_amperage_state_0",
			state: FloatPropertyState{
				Instance: AmperagePropertyInstance,
				Value:    0,
			},
			expectErrs: nil,
		},
		{
			name: "valid_voltage_state_50",
			state: FloatPropertyState{
				Instance: VoltagePropertyInstance,
				Value:    50,
			},
			expectErrs: nil,
		},
		{
			name: "valid_voltage_state_0",
			state: FloatPropertyState{
				Instance: VoltagePropertyInstance,
				Value:    0,
			},
			expectErrs: nil,
		},
		{
			name: "valid_temperature_state_50",
			state: FloatPropertyState{
				Instance: TemperaturePropertyInstance,
				Value:    50,
			},
			expectErrs: nil,
		},
		{
			name: "valid_temperature_state_0",
			state: FloatPropertyState{
				Instance: TemperaturePropertyInstance,
				Value:    0,
			},
			expectErrs: nil,
		},
		{
			name: "valid_temperature_state_-100",
			state: FloatPropertyState{
				Instance: TemperaturePropertyInstance,
				Value:    -100,
			},
			expectErrs: nil,
		},
		{
			name: "valid_power_state_50",
			state: FloatPropertyState{
				Instance: PowerPropertyInstance,
				Value:    50,
			},
			expectErrs: nil,
		},
		{
			name: "valid_power_state_0",
			state: FloatPropertyState{
				Instance: PowerPropertyInstance,
				Value:    0,
			},
			expectErrs: nil,
		},
		{
			name: "valid_pm1_density_state_0",
			state: FloatPropertyState{
				Instance: PM1DensityPropertyInstance,
				Value:    0,
			},
			expectErrs: nil,
		},
		{
			name: "valid_pm2.5_density_state_100",
			state: FloatPropertyState{
				Instance: PM2p5DensityPropertyInstance,
				Value:    100,
			},
			expectErrs: nil,
		},
		{
			name: "valid_pm10_density_state_322.22",
			state: FloatPropertyState{
				Instance: PM10DensityPropertyInstance,
				Value:    322.22,
			},
			expectErrs: nil,
		},
		{
			name: "valid_tvoc_state_0.1",
			state: FloatPropertyState{
				Instance: TvocPropertyInstance,
				Value:    0.1,
			},
			expectErrs: nil,
		},
		{
			name: "valid_pressure_state_0",
			state: FloatPropertyState{
				Instance: PressurePropertyInstance,
				Value:    0,
			},
			expectErrs: nil,
		},
		{
			name: "valid_pressure_state_700",
			state: FloatPropertyState{
				Instance: PressurePropertyInstance,
				Value:    700,
			},
			expectErrs: nil,
		},
		{
			name: "valid_battery_level_state_0",
			state: FloatPropertyState{
				Instance: BatteryLevelPropertyInstance,
				Value:    0,
			},
			expectErrs: nil,
		},
		{
			name: "valid_battery_level_state_50",
			state: FloatPropertyState{
				Instance: BatteryLevelPropertyInstance,
				Value:    50,
			},
			expectErrs: nil,
		},
		{
			name: "valid_battery_level_state_100",
			state: FloatPropertyState{
				Instance: BatteryLevelPropertyInstance,
				Value:    100,
			},
			expectErrs: nil,
		},
		{
			name: "valid_timer_state_100",
			state: FloatPropertyState{
				Instance: TimerPropertyInstance,
				Value:    100,
			},
			expectErrs: nil,
		},
		{
			name: "valid_smoke_concentration_state_0.1",
			state: FloatPropertyState{
				Instance: SmokeConcentrationPropertyInstance,
				Value:    0.01,
			},
			expectErrs: nil,
		},
		{
			name: "valid_gas_concentration_state_0.1",
			state: FloatPropertyState{
				Instance: GasConcentrationPropertyInstance,
				Value:    0.01,
			},
			expectErrs: nil,
		},
		//invalid
		{
			name: "invalid_water_level_state_322",
			state: FloatPropertyState{
				Instance: WaterLevelPropertyInstance,
				Value:    322.223,
			},
			expectErrs: valid.Errors{xerrors.New(`"water_level" instance value should be in range [0; 100], got 322.22`)},
		},
		{
			name: "invalid_water_level_state_-100",
			state: FloatPropertyState{
				Instance: WaterLevelPropertyInstance,
				Value:    -100,
			},
			expectErrs: valid.Errors{xerrors.New(`"water_level" instance value should be in range [0; 100], got -100.00`)},
		},
		{
			name: "invalid_humidity_state_322",
			state: FloatPropertyState{
				Instance: HumidityPropertyInstance,
				Value:    322.22,
			},
			expectErrs: valid.Errors{xerrors.New(`"humidity" instance value should be in range [0; 100], got 322.22`)},
		},
		{
			name: "invalid_humidity_state_-100",
			state: FloatPropertyState{
				Instance: HumidityPropertyInstance,
				Value:    -100,
			},
			expectErrs: valid.Errors{xerrors.New(`"humidity" instance value should be in range [0; 100], got -100.00`)},
		},
		{
			name: "invalid_co2_level_state_-100",
			state: FloatPropertyState{
				Instance: CO2LevelPropertyInstance,
				Value:    -100,
			},
			expectErrs: valid.Errors{xerrors.New(`"co2_level" instance value should be greater than 0, got -100.00`)},
		},
		{
			name: "invalid_amperage_state_-100",
			state: FloatPropertyState{
				Instance: AmperagePropertyInstance,
				Value:    -100,
			},
			expectErrs: valid.Errors{xerrors.New(`"amperage" instance value should be greater than 0, got -100.00`)},
		},
		{
			name: "invalid_voltage_state_-100",
			state: FloatPropertyState{
				Instance: VoltagePropertyInstance,
				Value:    -100,
			},
			expectErrs: valid.Errors{xerrors.New(`"voltage" instance value should be greater than 0, got -100.00`)},
		},
		{
			name: "invalid_power_state_-100",
			state: FloatPropertyState{
				Instance: PowerPropertyInstance,
				Value:    -100,
			},
			expectErrs: valid.Errors{xerrors.New(`"power" instance value should be greater than 0, got -100.00`)},
		},
		{
			name: "invalid_pressure_state_-100",
			state: FloatPropertyState{
				Instance: PressurePropertyInstance,
				Value:    -100,
			},
			expectErrs: valid.Errors{xerrors.New(`"pressure" instance value should be greater than 0, got -100.00`)},
		},
		{
			name: "invalid_battery_level_state_-100",
			state: FloatPropertyState{
				Instance: BatteryLevelPropertyInstance,
				Value:    -100,
			},
			expectErrs: valid.Errors{xerrors.New(`"battery_level" instance value should be in range [0; 100], got -100.00`)},
		},
		{
			name: "invalid_pm1_density_state_-100",
			state: FloatPropertyState{
				Instance: PM1DensityPropertyInstance,
				Value:    -100,
			},
			expectErrs: valid.Errors{xerrors.New(`"pm1_density" instance value should be greater than 0, got -100.00`)},
		},
		{
			name: "invalid_tvoc_state_-100",
			state: FloatPropertyState{
				Instance: TvocPropertyInstance,
				Value:    -100,
			},
			expectErrs: valid.Errors{xerrors.New(`"tvoc" instance value should be greater than 0, got -100.00`)},
		},
		{
			name: "invalid_timer_state_-1",
			state: FloatPropertyState{
				Instance: TimerPropertyInstance,
				Value:    -1,
			},
			expectErrs: valid.Errors{xerrors.New(`"timer" instance value should be greater than 0, got -1.00`)},
		},
		{
			name: "invalid_gas_concentration_state_-1",
			state: FloatPropertyState{
				Instance: GasConcentrationPropertyInstance,
				Value:    -1,
			},
			expectErrs: valid.Errors{xerrors.New(`"gas_concentration" instance value should be greater than 0, got -1.00`)},
		},
		{
			name: "invalid_smoke_concentration_state_-1",
			state: FloatPropertyState{
				Instance: SmokeConcentrationPropertyInstance,
				Value:    -1,
			},
			expectErrs: valid.Errors{xerrors.New(`"smoke_concentration" instance value should be greater than 0, got -1.00`)},
		},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			errs := valid.Struct(valid.NewValidationCtx(), tc.state)
			if tc.expectErrs == nil {
				assert.NoError(t, errs)
			} else {
				assert.EqualError(t, errs, tc.expectErrs.Error())
			}
		})
	}
}

func TestFloatPropertyParameters_Validate(t *testing.T) {
	testCases := []struct {
		name       string
		parameters IPropertyParameters
		expectErrs valid.Errors
	}{
		//valid
		{
			name: "valid_water_level_parameters",
			parameters: FloatPropertyParameters{
				Instance: WaterLevelPropertyInstance,
				Unit:     UnitPercent,
			},
			expectErrs: nil,
		},
		{
			name: "valid_humidity_parameters",
			parameters: FloatPropertyParameters{
				Instance: HumidityPropertyInstance,
				Unit:     UnitPercent,
			},
			expectErrs: nil,
		},
		{
			name: "valid_co2_parameters",
			parameters: FloatPropertyParameters{
				Instance: CO2LevelPropertyInstance,
				Unit:     UnitPPM,
			},
			expectErrs: nil,
		},
		{
			name: "valid_amperage_parameters",
			parameters: FloatPropertyParameters{
				Instance: AmperagePropertyInstance,
				Unit:     UnitAmpere,
			},
			expectErrs: nil,
		},
		{
			name: "valid_temperature_parameters_kelvin",
			parameters: FloatPropertyParameters{
				Instance: TemperaturePropertyInstance,
				Unit:     UnitTemperatureKelvin,
			},
			expectErrs: nil,
		},
		{
			name: "valid_temperature_parameters_celsius",
			parameters: FloatPropertyParameters{
				Instance: TemperaturePropertyInstance,
				Unit:     UnitTemperatureCelsius,
			},
			expectErrs: nil,
		},
		{
			name: "valid_voltage_parameters",
			parameters: FloatPropertyParameters{
				Instance: VoltagePropertyInstance,
				Unit:     UnitVolt,
			},
			expectErrs: nil,
		},
		{
			name: "valid_power_parameters",
			parameters: FloatPropertyParameters{
				Instance: PowerPropertyInstance,
				Unit:     UnitWatt,
			},
			expectErrs: nil,
		},
		{
			name: "valid_pm1_density_parameters",
			parameters: FloatPropertyParameters{
				Instance: PM1DensityPropertyInstance,
				Unit:     UnitDensityMcgM3,
			},
			expectErrs: nil,
		},
		{
			name: "valid_pm2.5_density_parameters",
			parameters: FloatPropertyParameters{
				Instance: PM2p5DensityPropertyInstance,
				Unit:     UnitDensityMcgM3,
			},
			expectErrs: nil,
		},
		{
			name: "valid_pm10_density_parameters",
			parameters: FloatPropertyParameters{
				Instance: PM10DensityPropertyInstance,
				Unit:     UnitDensityMcgM3,
			},
			expectErrs: nil,
		},
		{
			name: "valid_tvoc_parameters",
			parameters: FloatPropertyParameters{
				Instance: TvocPropertyInstance,
				Unit:     UnitDensityMcgM3,
			},
			expectErrs: nil,
		},
		{
			name: "valid_pressure_parameters",
			parameters: FloatPropertyParameters{
				Instance: PressurePropertyInstance,
				Unit:     UnitPressureMmHg,
			},
			expectErrs: nil,
		},
		{
			name: "valid_pressure_parameters",
			parameters: FloatPropertyParameters{
				Instance: PressurePropertyInstance,
				Unit:     UnitPressurePascal,
			},
			expectErrs: nil,
		},
		{
			name: "valid_pressure_parameters",
			parameters: FloatPropertyParameters{
				Instance: PressurePropertyInstance,
				Unit:     UnitPressureBar,
			},
			expectErrs: nil,
		},
		{
			name: "valid_battery_level_parameters",
			parameters: FloatPropertyParameters{
				Instance: BatteryLevelPropertyInstance,
				Unit:     UnitPercent,
			},
			expectErrs: nil,
		},
		{
			name: "valid_timer_parameters",
			parameters: FloatPropertyParameters{
				Instance: TimerPropertyInstance,
				Unit:     UnitTimeSeconds,
			},
			expectErrs: nil,
		},
		{
			name: "valid_smoke_concentration_parameters",
			parameters: FloatPropertyParameters{
				Instance: SmokeConcentrationPropertyInstance,
				Unit:     UnitPercent,
			},
			expectErrs: nil,
		},
		{
			name: "valid_gas_concentration_parameters",
			parameters: FloatPropertyParameters{
				Instance: GasConcentrationPropertyInstance,
				Unit:     UnitPercent,
			},
			expectErrs: nil,
		},
		//invalid
		{
			name: "invalid_water_level_parameters",
			parameters: FloatPropertyParameters{
				Instance: WaterLevelPropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"water_level" instance unit should be "unit.percent", not "invalid_unit"`)},
		},
		{
			name: "invalid_humidity_parameters",
			parameters: FloatPropertyParameters{
				Instance: HumidityPropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"humidity" instance unit should be "unit.percent", not "invalid_unit"`)},
		},
		{
			name: "invalid_co2_level_parameters",
			parameters: FloatPropertyParameters{
				Instance: CO2LevelPropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"co2_level" instance unit should be "unit.ppm", not "invalid_unit"`)},
		},
		{
			name: "invalid_temperature_parameters",
			parameters: FloatPropertyParameters{
				Instance: TemperaturePropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"temperature" instance unit should be either "unit.temperature.kelvin" or "unit.temperature.celsius", not "invalid_unit"`)},
		},
		{
			name: "invalid_amperage_parameters",
			parameters: FloatPropertyParameters{
				Instance: AmperagePropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"amperage" instance unit should be "unit.ampere", not "invalid_unit"`)},
		},
		{
			name: "invalid_voltage_parameters",
			parameters: FloatPropertyParameters{
				Instance: VoltagePropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"voltage" instance unit should be "unit.volt", not "invalid_unit"`)},
		},
		{
			name: "invalid_power_parameters",
			parameters: FloatPropertyParameters{
				Instance: PowerPropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"power" instance unit should be "unit.watt", not "invalid_unit"`)},
		},
		{
			name: "invalid_pm1_density_parameters",
			parameters: FloatPropertyParameters{
				Instance: PM1DensityPropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"pm1_density" instance unit should be either "unit.density.mcg_m3" or "unit.ppm", not "invalid_unit"`)},
		},
		{
			name: "invalid_pm2.5_density_parameters",
			parameters: FloatPropertyParameters{
				Instance: PM2p5DensityPropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"pm2.5_density" instance unit should be either "unit.density.mcg_m3" or "unit.ppm", not "invalid_unit"`)},
		},
		{
			name: "invalid_pm10_density_parameters",
			parameters: FloatPropertyParameters{
				Instance: PM10DensityPropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"pm10_density" instance unit should be either "unit.density.mcg_m3" or "unit.ppm", not "invalid_unit"`)},
		},
		{
			name: "invalid_tvoc_parameters",
			parameters: FloatPropertyParameters{
				Instance: TvocPropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"tvoc" instance unit should be either "unit.density.mcg_m3" or "unit.ppm", not "invalid_unit"`)},
		},
		{
			name: "invalid_pressure_parameters",
			parameters: FloatPropertyParameters{
				Instance: PressurePropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"pressure" instance unit should be either "unit.pressure.bar", "unit.pressure.mmhg", "unit.pressure.pascal", or "unit.pressure.atm", not "invalid_unit"`)},
		},
		{
			name: "invalid_battery_level_parameters",
			parameters: FloatPropertyParameters{
				Instance: BatteryLevelPropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"battery_level" instance unit should be "unit.percent", not "invalid_unit"`)},
		},
		{
			name: "invalid_timer_parameters",
			parameters: FloatPropertyParameters{
				Instance: TimerPropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"timer" instance unit should be "unit.time.seconds", not "invalid_unit"`)},
		},
		{
			name: "invalid_smoke_concentration_parameters",
			parameters: FloatPropertyParameters{
				Instance: SmokeConcentrationPropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"smoke_concentration" instance unit should be "unit.percent", not "invalid_unit"`)},
		},
		{
			name: "invalid_gas_concentration_parameters",
			parameters: FloatPropertyParameters{
				Instance: GasConcentrationPropertyInstance,
				Unit:     "invalid_unit",
			},
			expectErrs: valid.Errors{xerrors.New(`"gas_concentration" instance unit should be "unit.percent", not "invalid_unit"`)},
		},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			errs := valid.Struct(valid.NewValidationCtx(), tc.parameters)
			if tc.expectErrs == nil {
				assert.NoError(t, errs)
			} else {
				assert.EqualError(t, errs, tc.expectErrs.Error())
			}
		})
	}
}

func TestPropertyThresholdsState(t *testing.T) {
	assert.NotEmpty(t, FloatPropertyThresholds)

	for instance, val := range FloatPropertyThresholds {
		intervals := val.Intervals
		t.Run(instance.String(), func(t *testing.T) {
			assert.NotEmpty(t, intervals)
			assert.Equal(t, float64(0), intervals[0].Start, "start of first interval must be zero")
			assert.Equal(t, math.Inf(1), intervals[len(intervals)-1].End, "end of the last interval must me maxFloat64")

			lastEnd := intervals[0].Start
			for _, interval := range intervals {
				assert.True(t, interval.Start == lastEnd, fmt.Sprintf("interval start %f must be equal previous interval end %f", interval.Start, lastEnd))
				assert.True(t, interval.Start < interval.End, fmt.Sprintf("start %f must be less then end %f", interval.Start, interval.End))
				lastEnd = interval.End
			}
		})
	}
}
