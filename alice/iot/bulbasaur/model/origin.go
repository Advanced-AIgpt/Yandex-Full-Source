package model

import (
	"context"
	"encoding/json"
)

type SurfaceType string

var (
	SpeakerSurfaceType    SurfaceType = "speaker"
	StereopairSurfaceType SurfaceType = "stereopair"
	SearchAppSurfaceType  SurfaceType = "searchapp"
	APISurfaceType        SurfaceType = "api"
	CallbackSurfaceType   SurfaceType = "callback"
)

type Origin struct {
	SurfaceParameters SurfaceParameters
	User              User

	// xxx(galecore): if(when) new oneof values will arrive, think of type+parameters for them
	// if you want to add another oneof value, don't add it via pointer
	// this is just a shortcut
	ScenarioLaunchInfo *ScenarioLaunchInfo
}

func (o *Origin) MarshalJSON() ([]byte, error) {
	rawOrigin := struct {
		SurfaceType        SurfaceType         `json:"surface_type"`
		SurfaceParameters  SurfaceParameters   `json:"surface_parameters"`
		UserID             uint64              `json:"user_id"`
		ScenarioLaunchInfo *ScenarioLaunchInfo `json:"scenario_launch_info,omitempty"`
	}{
		SurfaceType:        o.SurfaceParameters.SurfaceType(),
		SurfaceParameters:  o.SurfaceParameters,
		UserID:             o.User.ID,
		ScenarioLaunchInfo: o.ScenarioLaunchInfo,
	}
	return json.Marshal(rawOrigin)
}

func (o *Origin) IsSpeakerDeviceOrigin(targetID string) bool {
	speakerSurface, isSpeakerOrigin := o.SurfaceParameters.(SpeakerSurfaceParameters)
	if !isSpeakerOrigin {
		return false
	}
	return speakerSurface.ID == targetID
}

func (o *Origin) Clone() Origin {
	return Origin{
		o.SurfaceParameters.Clone(),
		o.User.Clone(),
		o.ScenarioLaunchInfo.Clone(),
	}
}

func (o *Origin) ToSharedOrigin(sharedUserID uint64) Origin {
	result := o.Clone()
	result.User = User{ID: sharedUserID}
	return result
}

func (o *Origin) IsEmpty() bool {
	if o == nil {
		return true
	}
	return *o == Origin{}
}

func NewOrigin(ctx context.Context, parameters SurfaceParameters, user User) Origin {
	return Origin{
		SurfaceParameters: parameters,
		User:              user,
	}
}

type SurfaceParameters interface {
	SurfaceType() SurfaceType
	Clone() SurfaceParameters
}

type SpeakerSurfaceParameters struct {
	ID       string
	Platform string
}

func (s SpeakerSurfaceParameters) SurfaceType() SurfaceType {
	return SpeakerSurfaceType
}

func (s SpeakerSurfaceParameters) Clone() SurfaceParameters {
	return SpeakerSurfaceParameters{
		ID:       s.ID,
		Platform: s.Platform,
	}
}

type StereopairSurfaceParameters struct {
	ID string
}

func (s StereopairSurfaceParameters) SurfaceType() SurfaceType {
	return StereopairSurfaceType
}

func (s StereopairSurfaceParameters) Clone() SurfaceParameters {
	return StereopairSurfaceParameters{
		ID: s.ID,
	}
}

type SearchAppSurfaceParameters struct{}

func (s SearchAppSurfaceParameters) SurfaceType() SurfaceType {
	return SearchAppSurfaceType
}

func (s SearchAppSurfaceParameters) Clone() SurfaceParameters {
	return SearchAppSurfaceParameters{}
}

type APISurfaceParameters struct{}

func (a APISurfaceParameters) SurfaceType() SurfaceType {
	return APISurfaceType
}

func (a APISurfaceParameters) Clone() SurfaceParameters {
	return APISurfaceParameters{}
}

type CallbackSurfaceParameters struct{}

func (c CallbackSurfaceParameters) SurfaceType() SurfaceType {
	return CallbackSurfaceType
}

func (c CallbackSurfaceParameters) Clone() SurfaceParameters {
	return CallbackSurfaceParameters{}
}

// ScenarioLaunchInfo is origin-related
type ScenarioLaunchInfo struct {
	ID        string
	StepIndex int
}

func (s *ScenarioLaunchInfo) Clone() *ScenarioLaunchInfo {
	if s == nil {
		return nil
	}
	return &ScenarioLaunchInfo{
		s.ID,
		s.StepIndex,
	}
}
