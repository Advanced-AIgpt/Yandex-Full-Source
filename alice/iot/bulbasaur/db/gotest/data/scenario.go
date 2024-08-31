package data

import (
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/library/go/random"
	"a.yandex-team.ru/alice/library/go/tools"
)

func GenerateScenario(name string, devices []model.Device) (s model.Scenario) {
	for strings.TrimSpace(name) == "" {
		name = testing.RandCyrillicWithNumbersString(random.RandRange(1, 100))
	}

	s.Name = model.ScenarioName(name)
	s.Icon = model.ScenarioIcon(random.Choice(model.KnownScenarioIcons))
	s.Triggers = []model.ScenarioTrigger{
		model.VoiceScenarioTrigger{Phrase: string(s.Name)},
	}

	s.RequestedSpeakerCapabilities = make([]model.ScenarioCapability, 0)
	if random.FlipCoin() {
		quasarServerActionCap := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		quasarServerActionCap.SetParameters(model.QuasarServerActionCapabilityParameters{
			Instance: model.QuasarServerActionCapabilityInstance(random.Choice(model.KnownQuasarServerActionInstances)),
		})
		quasarServerActionState := generateScenarioCapabilityState(quasarServerActionCap)
		s.RequestedSpeakerCapabilities = append(s.RequestedSpeakerCapabilities,
			model.ScenarioCapability{
				Type:  model.QuasarServerActionCapabilityType,
				State: quasarServerActionState,
			})
	}

	s.Devices = make([]model.ScenarioDevice, 0, len(devices))
	for _, d := range devices {
		sd := model.ScenarioDevice{
			ID:           d.ID,
			Capabilities: make([]model.ScenarioCapability, len(d.Capabilities)),
		}
		for i, c := range d.Capabilities {
			sd.Capabilities[i] = model.ScenarioCapability{
				Type:  c.Type(),
				State: generateScenarioCapabilityState(c),
			}
		}

		s.Devices = append(s.Devices, sd)
	}

	if random.FlipCoin() {
		qsai := random.Choice([]string{string(model.PhraseActionCapabilityInstance), string(model.TextActionCapabilityInstance)})
		quasarCap := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		quasarParam := model.MakeQuasarServerActionParametersByInstance(model.QuasarServerActionCapabilityInstance(qsai))
		quasarCap.SetParameters(quasarParam)
		s.RequestedSpeakerCapabilities = append(s.RequestedSpeakerCapabilities, model.ScenarioCapability{
			Type:  quasarCap.Type(),
			State: generateScenarioCapabilityState(quasarCap),
		})
	}

	s.Steps = s.ScenarioSteps(devices)

	s.IsActive = true

	return s
}

func generateScenarioCapabilityState(c model.ICapability) model.ICapabilityState {
	switch c.Type() {
	case model.OnOffCapabilityType:
		state := generateCapabilityState(c)
		if onOffState, ok := state.(model.OnOffCapabilityState); ok && !c.Retrievable() {
			onOffState.Value = true
		}
		return state

	case model.ColorSettingCapabilityType:
		return generateCapabilityState(c)

	case model.RangeCapabilityType:
		if params, ok := c.Parameters().(model.RangeCapabilityParameters); ok {
			if params.RandomAccess && random.FlipCoin() {
				return generateCapabilityState(c)
			} else {
				state := model.RangeCapabilityState{Instance: params.Instance}
				state.Relative = tools.AOB(true)

				if params.Range != nil {
					if random.FlipCoin() {
						state.Value = params.Range.Precision
					} else {
						state.Value = -params.Range.Precision
					}
				} else {
					if random.FlipCoin() {
						state.Value = 1
					} else {
						state.Value = -1
					}
				}

				return state
			}
		}

	case model.ModeCapabilityType:
		// TODO: QUASAR-4226 add parameters.ordered support
		return generateCapabilityState(c)

	case model.ToggleCapabilityType:
		state := generateCapabilityState(c)
		if toggleState, ok := state.(model.ToggleCapabilityState); ok && !c.Retrievable() {
			toggleState.Value = true
		}
		return state

	case model.CustomButtonCapabilityType:
		state := generateCapabilityState(c)
		if cbState, ok := state.(model.CustomButtonCapabilityState); ok && !c.Retrievable() {
			cbState.Value = true
		}
		return state

	case model.QuasarServerActionCapabilityType:
		return generateCapabilityState(c)

	case model.QuasarCapabilityType:
		return generateCapabilityState(c)

	case model.VideoStreamCapabilityType:
		return generateCapabilityState(c)
	}
	return nil
}
