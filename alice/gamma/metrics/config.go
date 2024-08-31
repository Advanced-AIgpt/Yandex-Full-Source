package metrics

import (
	"a.yandex-team.ru/alice/gamma/metrics/generic"
)

type Config struct {
	TimersConfig map[string]generic.TimerConfig `yaml:"timers"`
	HistsConfig  map[string]generic.HistConfig  `yaml:"histograms"`
}

func (config *Config) SetDefaultTimerConfig(defaultConfig generic.TimerConfig) {
	config.TimersConfig["default"] = defaultConfig
}

func (config *Config) GetTimerConfig(name string) generic.TimerConfig {
	if timerConfig, ok := config.TimersConfig[name]; ok {
		return timerConfig
	}
	return config.TimersConfig["default"]
}

func (config *Config) SetDefaultHistConfig(defaultConfig generic.HistConfig) {
	config.HistsConfig["default"] = defaultConfig
}

func (config *Config) GetHistConfig(name string) generic.HistConfig {
	if histConfig, ok := config.HistsConfig[name]; ok {
		return histConfig
	}
	return config.HistsConfig["default"]
}
