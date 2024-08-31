package sdk

import (
	"a.yandex-team.ru/alice/library/go/libmegamind"
)

type InputType string

const (
	TypedSemanticFrameInputType InputType = "typed_semantic_frame"
	SemanticFrameInputType      InputType = "semantic_frame"
	CallbackInputType           InputType = "callback"
)

type SupportedInputs struct {
	SupportedFrames    []libmegamind.SemanticFrameName
	SupportedCallbacks []libmegamind.CallbackName
}

type Input interface {
	Type() InputType
	GetFirstFrame() *libmegamind.SemanticFrame
	GetFrames() libmegamind.SemanticFrames
	GetCallback() *libmegamind.Callback
}

type input struct {
	frames   libmegamind.SemanticFrames
	callback *libmegamind.Callback
}

func (i *input) Type() InputType {
	switch {
	case len(i.frames) > 0:
		if i.frames[0].Frame.GetTypedSemanticFrame() != nil {
			return TypedSemanticFrameInputType
		}
		return SemanticFrameInputType
	case i.callback != nil:
		return CallbackInputType
	}
	return ""
}

func (i *input) GetFirstFrame() *libmegamind.SemanticFrame {
	if len(i.frames) == 0 {
		return nil
	}
	return &i.frames[0]
}

func (i *input) GetFrames() libmegamind.SemanticFrames {
	return i.frames
}

func (i *input) GetCallback() *libmegamind.Callback {
	return i.callback
}

func InputFrames(frames ...libmegamind.SemanticFrame) Input {
	return &input{frames: frames}
}

func InputCallback(callback libmegamind.Callback) Input {
	return &input{callback: &callback}
}
