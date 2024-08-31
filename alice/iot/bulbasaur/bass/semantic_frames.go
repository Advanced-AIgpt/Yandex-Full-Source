package bass

import (
	"encoding/json"

	"google.golang.org/protobuf/encoding/protojson"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type ServiceSemanticFrameData struct {
	TypedSemanticFrame ITypedSemanticFrame        `json:"typed_semantic_frame"`
	Utterance          string                     `json:"utterance"`
	AnalyticsData      SemanticFrameAnalyticsData `json:"analytics"`
}

type SemanticFrameAnalyticsData struct {
	ProductScenario string `json:"product_scenario"`
	Origin          string `json:"origin"`
	Purpose         string `json:"purpose"`
}

func (sfad SemanticFrameAnalyticsData) NotFullyAssigned() bool {
	return len(sfad.Purpose) == 0 || len(sfad.Origin) == 0 || len(sfad.ProductScenario) == 0
}

type TypedSemanticFrameStringSlot struct {
	StringValue string `json:"string_value"`
}

type TypedSemanticFrameUInt32Slot struct {
	UInt32Value uint32 `json:"uint32_value"`
}

type TypedSemanticFrameCapabilityActionSlot struct {
	CapabilityActionValue TypedSemanticFrameCapabilityActionValue `json:"capability_action_value"`
}

type TypedSemanticFrameCapabilityActionValue struct {
	Type  model.CapabilityType
	State model.ICapabilityState
}

func (v *TypedSemanticFrameCapabilityActionValue) FromSpeakerActionCapabilityValue(capabilityActionValue frames.SpeakerActionCapabilityValue) {
	v.Type = capabilityActionValue.Type
	v.State = capabilityActionValue.State.Clone()
}

func (v TypedSemanticFrameCapabilityActionValue) MarshalJSON() ([]byte, error) {
	// every day we stray further away from god
	type quasarStateValue struct {
		MusicPlay      interface{} `json:"music_play_value,omitempty"`
		News           interface{} `json:"news_value,omitempty"`
		SoundPlay      interface{} `json:"sound_play_value,omitempty"`
		StopEverything interface{} `json:"stop_everything_value,omitempty"`
		Volume         interface{} `json:"volume_value,omitempty"`
		Weather        interface{} `json:"weather_value,omitempty"`
		TTS            interface{} `json:"tts_value,omitempty"`
		AliceShow      interface{} `json:"alice_show_value,omitempty"`
	}

	var rawValue struct {
		Type                    string      `json:"type"`
		OnOffState              interface{} `json:"on_off_capability_state,omitempty"`
		ColorSettingState       interface{} `json:"color_setting_capability_state,omitempty"`
		ModeState               interface{} `json:"mode_capability_state,omitempty"`
		RangeState              interface{} `json:"range_capability_state,omitempty"`
		ToggleState             interface{} `json:"toggle_capability_state,omitempty"`
		CustomButtonState       interface{} `json:"custom_button_capability_state,omitempty"`
		QuasarServerActionState interface{} `json:"quasar_server_action_capability_state,omitempty"`
		QuasarState             *struct {
			Instance string `json:"instance"`
			quasarStateValue
		} `json:"quasar_capability_state,omitempty"`
	}
	switch v.Type {
	case model.OnOffCapabilityType:
		rawValue.Type = "OnOffCapabilityType"
		onOffState := v.State.(model.OnOffCapabilityState)
		rawValue.OnOffState = &struct {
			Instance string `json:"instance"`
			Value    bool   `json:"value"`
		}{
			Instance: onOffState.Instance.String(),
			Value:    onOffState.Value,
		}
	case model.ColorSettingCapabilityType:
		rawValue.Type = "ColorSettingCapabilityType"
		colorSettingState := v.State.(model.ColorSettingCapabilityState)

		type colorSettingValue struct {
			TemperatureK *int32  `json:"temperature_k,omitempty"`
			RGB          *int32  `json:"rgb,omitempty"`
			ColorSceneID *string `json:"color_scene_id,omitempty"`
			HSV          *struct {
				H int32 `json:"h"`
				S int32 `json:"s"`
				V int32 `json:"v"`
			} `json:"hsv,omitempty"`
		}

		rawState := &struct {
			Instance string `json:"instance"`
			colorSettingValue
		}{
			Instance: v.State.GetInstance(),
		}
		switch value := colorSettingState.Value.(type) {
		case model.TemperatureK:
			rawState.TemperatureK = ptr.Int32(int32(value))
		case model.ColorSceneID:
			rawState.ColorSceneID = ptr.String(string(value))
		case model.HSV:
			rawState.HSV = &struct {
				H int32 `json:"h"`
				S int32 `json:"s"`
				V int32 `json:"v"`
			}{
				H: int32(value.H),
				S: int32(value.S),
				V: int32(value.V),
			}
		case model.RGB:
			rawState.RGB = ptr.Int32(int32(value))
		}
		rawValue.ColorSettingState = rawState
	case model.ModeCapabilityType:
		rawValue.Type = "ModeCapabilityType"
		modeState := v.State.(model.ModeCapabilityState)
		rawValue.ModeState = &struct {
			Instance string `json:"instance"`
			Value    string `json:"value"`
		}{
			Instance: modeState.Instance.String(),
			Value:    string(modeState.Value),
		}
	case model.RangeCapabilityType:
		rawValue.Type = "RangeCapabilityType"
		rangeState := v.State.(model.RangeCapabilityState)
		var relative *struct {
			IsRelative bool `json:"is_relative"`
		}
		if rangeState.Relative != nil {
			relative = &struct {
				IsRelative bool `json:"is_relative"`
			}{IsRelative: *rangeState.Relative}
		}
		rawValue.RangeState = &struct {
			Instance string  `json:"instance"`
			Value    float64 `json:"value"`
			Relative *struct {
				IsRelative bool `json:"is_relative"`
			} `json:"relative,omitempty"`
		}{
			Instance: rangeState.Instance.String(),
			Value:    rangeState.Value,
			Relative: relative,
		}
	case model.ToggleCapabilityType:
		rawValue.Type = "ToggleCapabilityType"
		toggleState := v.State.(model.ToggleCapabilityState)
		rawValue.ToggleState = &struct {
			Instance string `json:"instance"`
			Value    bool   `json:"value"`
		}{
			Instance: toggleState.Instance.String(),
			Value:    toggleState.Value,
		}
	case model.CustomButtonCapabilityType:
		rawValue.Type = "CustomButtonCapabilityType"
		customButtonState := v.State.(model.CustomButtonCapabilityState)
		rawValue.CustomButtonState = &struct {
			Instance string `json:"instance"`
			Value    bool   `json:"value"`
		}{
			Instance: string(customButtonState.Instance),
			Value:    customButtonState.Value,
		}
	case model.QuasarServerActionCapabilityType:
		rawValue.Type = "QuasarServerActionCapabilityType"
		serverActionState := v.State.(model.QuasarServerActionCapabilityState)
		rawValue.QuasarServerActionState = &struct {
			Instance string `json:"instance"`
			Value    string `json:"value"`
		}{
			Instance: serverActionState.Instance.String(),
			Value:    serverActionState.Value,
		}
	case model.QuasarCapabilityType:
		rawValue.Type = "QuasarCapabilityType"
		quasarState := v.State.(model.QuasarCapabilityState)
		rawValue.QuasarState = &struct {
			Instance string `json:"instance"`
			quasarStateValue
		}{
			Instance: v.State.GetInstance(),
		}
		switch value := quasarState.Value.(type) {
		case model.WeatherQuasarCapabilityValue:
			type weatherWhere struct {
				Longitude    float64 `json:"longitude"`
				Latitude     float64 `json:"latitude"`
				Address      string  `json:"address"`
				ShortAddress string  `json:"short_address"`
			}
			type weatherHousehold struct {
				ID   string `json:"id"`
				Name string `json:"name"`
			}
			var where *weatherWhere
			if value.Where != nil {
				where = &weatherWhere{
					Longitude:    value.Where.Longitude,
					Latitude:     value.Where.Latitude,
					Address:      value.Where.Address,
					ShortAddress: value.Where.ShortAddress,
				}
			}
			var household *weatherHousehold
			if value.Household != nil {
				household = &weatherHousehold{
					ID:   value.Household.ID,
					Name: value.Household.Name,
				}
			}
			rawValue.QuasarState.Weather = &struct {
				Where     *weatherWhere     `json:"where,omitempty"`
				Household *weatherHousehold `json:"household,omitempty"`
			}{
				Where:     where,
				Household: household,
			}
		case model.VolumeQuasarCapabilityValue:
			type volume struct {
				Value    int  `json:"value"`
				Relative bool `json:"relative,omitempty"`
			}
			rawValue.QuasarState.Volume = &volume{
				Value:    value.Value,
				Relative: value.Relative,
			}
		case model.MusicPlayQuasarCapabilityValue:
			type musicObject struct {
				ID   string `json:"id"`
				Name string `json:"name"`
				Type string `json:"type"`
			}
			var object *musicObject
			if value.Object != nil {
				object = &musicObject{
					ID:   value.Object.ID,
					Name: value.Object.Name,
					Type: string(value.Object.Type),
				}
			}
			rawValue.QuasarState.MusicPlay = &struct {
				Object           *musicObject `json:"object,omitempty"`
				SearchText       string       `json:"search_text,omitempty"`
				PlayInBackground bool         `json:"play_in_background,omitempty"`
			}{
				Object:           object,
				SearchText:       value.SearchText,
				PlayInBackground: value.PlayInBackground,
			}
		case model.NewsQuasarCapabilityValue:
			rawValue.QuasarState.News = &struct {
				Topic            string `json:"topic"`
				Provider         string `json:"provider,omitempty"`
				PlayInBackground bool   `json:"play_in_background,omitempty"`
			}{
				Topic:            string(value.Topic),
				Provider:         value.Provider,
				PlayInBackground: value.PlayInBackground,
			}
		case model.SoundPlayQuasarCapabilityValue:
			rawValue.QuasarState.SoundPlay = &struct {
				Sound string `json:"sound"`
			}{
				Sound: string(value.Sound),
			}
		case model.StopEverythingQuasarCapabilityValue:
			rawValue.QuasarState.StopEverything = &struct{}{}
		case model.TTSQuasarCapabilityValue:
			rawValue.QuasarState.TTS = &struct {
				Text string `json:"text"`
			}{
				Text: value.Text,
			}
		case model.AliceShowQuasarCapabilityValue:
			rawValue.QuasarState.AliceShow = &struct{}{}
		default:
			return nil, xerrors.New("failed to marshal quasar capability value: unknown value type")
		}
	default:
		return nil, xerrors.New("unknown capability type: %q")
	}
	return json.Marshal(rawValue)
}

type SemanticFrameType string

type IoTBroadcastSuccessTypedSemanticFrame struct {
	DevicesID  TypedSemanticFrameStringSlot `json:"devices_id"`
	ProductIDs TypedSemanticFrameStringSlot `json:"product_ids"`
}

func (ibstsf IoTBroadcastSuccessTypedSemanticFrame) Type() SemanticFrameType {
	return BroadcastSuccessSemanticFrame
}

func (ibstsf IoTBroadcastSuccessTypedSemanticFrame) MarshalJSON() ([]byte, error) {
	var rawTypedSemanticFrame struct {
		IoTBroadcastSuccessFrame struct {
			DevicesID  TypedSemanticFrameStringSlot `json:"devices_id"`
			ProductIDs TypedSemanticFrameStringSlot `json:"product_ids"`
		} `json:"iot_broadcast_success"`
	}
	rawTypedSemanticFrame.IoTBroadcastSuccessFrame.DevicesID = ibstsf.DevicesID
	rawTypedSemanticFrame.IoTBroadcastSuccessFrame.ProductIDs = ibstsf.ProductIDs
	return json.Marshal(rawTypedSemanticFrame)
}

type IoTBroadcastFailureTypedSemanticFrame struct {
	TimeoutMs TypedSemanticFrameUInt32Slot `json:"timeout_ms"`
	Reason    TypedSemanticFrameStringSlot `json:"reason"`
}

func (ibftsf IoTBroadcastFailureTypedSemanticFrame) Type() SemanticFrameType {
	return BroadcastFailureSemanticFrame
}

func (ibftsf IoTBroadcastFailureTypedSemanticFrame) MarshalJSON() ([]byte, error) {
	var rawTypedSemanticFrame struct {
		IoTBroadcastFailureFrame struct {
			TimeoutMs TypedSemanticFrameUInt32Slot `json:"timeout_ms"`
			Reason    TypedSemanticFrameStringSlot `json:"reason"`
		} `json:"iot_broadcast_failure"`
	}
	rawTypedSemanticFrame.IoTBroadcastFailureFrame.TimeoutMs = ibftsf.TimeoutMs
	rawTypedSemanticFrame.IoTBroadcastFailureFrame.Reason = ibftsf.Reason
	return json.Marshal(rawTypedSemanticFrame)
}

type IoTDiscoverySuccessTypedSemanticFrame struct {
	DeviceIDs  TypedSemanticFrameStringSlot `json:"device_ids"`
	ProductIDs TypedSemanticFrameStringSlot `json:"product_ids"`
	DeviceType TypedSemanticFrameStringSlot `json:"device_type"`
}

func (idstsf IoTDiscoverySuccessTypedSemanticFrame) Type() SemanticFrameType {
	return DiscoverySuccessSemanticFrame
}

func (idstsf IoTDiscoverySuccessTypedSemanticFrame) MarshalJSON() ([]byte, error) {
	var rawTypedSemanticFrame struct {
		IoTDiscoverySuccessFrame struct {
			DeviceIDs  TypedSemanticFrameStringSlot `json:"device_ids"`
			ProductIDs TypedSemanticFrameStringSlot `json:"product_ids"`
			DeviceType TypedSemanticFrameStringSlot `json:"device_type"`
		} `json:"iot_discovery_success"`
	}
	rawTypedSemanticFrame.IoTDiscoverySuccessFrame.DeviceIDs = idstsf.DeviceIDs
	rawTypedSemanticFrame.IoTDiscoverySuccessFrame.ProductIDs = idstsf.ProductIDs
	rawTypedSemanticFrame.IoTDiscoverySuccessFrame.DeviceType = idstsf.DeviceType
	return json.Marshal(rawTypedSemanticFrame)
}

type IoTSpeakerActionTypedSemanticFrame frames.SpeakerActionFrame

func (f IoTSpeakerActionTypedSemanticFrame) Type() SemanticFrameType {
	return IoTScenarioSpeakerActionSemanticFrame
}

func (f IoTSpeakerActionTypedSemanticFrame) MarshalJSON() ([]byte, error) {
	var rawTypedSemanticFrame struct {
		IoTSpeakerActionFrame struct {
			LaunchID         TypedSemanticFrameStringSlot           `json:"launch_id"`
			StepIndex        TypedSemanticFrameUInt32Slot           `json:"step_index"`
			CapabilityAction TypedSemanticFrameCapabilityActionSlot `json:"capability_action"`
		} `json:"iot_scenario_speaker_action_semantic_frame"`
	}
	rawTypedSemanticFrame.IoTSpeakerActionFrame.LaunchID.StringValue = f.LaunchID
	rawTypedSemanticFrame.IoTSpeakerActionFrame.StepIndex.UInt32Value = f.StepIndex
	rawTypedSemanticFrame.IoTSpeakerActionFrame.CapabilityAction.CapabilityActionValue.FromSpeakerActionCapabilityValue(f.CapabilityAction)
	return json.Marshal(rawTypedSemanticFrame)
}

type YandexIOActionTypedSemanticFrame frames.EndpointActionsFrame

func (f YandexIOActionTypedSemanticFrame) Type() SemanticFrameType {
	return YandexIOActionSemanticFrame
}

func (f YandexIOActionTypedSemanticFrame) MarshalJSON() ([]byte, error) {
	frame := frames.EndpointActionsFrame(f)
	return protojson.Marshal(frame.ToTypedSemanticFrame())
}

type IoTScenarioStepActionsFrame frames.ScenarioStepActionsFrame

func (f IoTScenarioStepActionsFrame) Type() SemanticFrameType {
	return ScenarioStepActionsSemanticFrame
}

func (f IoTScenarioStepActionsFrame) MarshalJSON() ([]byte, error) {
	frame := frames.ScenarioStepActionsFrame(f)
	return protojson.Marshal(frame.ToTypedSemanticFrame())
}
