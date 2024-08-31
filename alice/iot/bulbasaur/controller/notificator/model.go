package notificator

import (
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/known/structpb"
	"google.golang.org/protobuf/types/known/wrapperspb"

	commonpb "a.yandex-team.ru/alice/megamind/protos/common"
	speechkitpb "a.yandex-team.ru/alice/megamind/protos/speechkit"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type TSF interface {
	ToTypedSemanticFrame() *commonpb.TTypedSemanticFrame
}

type SpeechkitDirective interface {
	// EndpointID of the directive
	// Should not appear in payload
	EndpointID() string

	// SpeechkitName of the directive.
	// Should be taken from SpeechkitName message option
	// example: https://a.yandex-team.ru/arc_vcs/alice/protos/endpoint/capability.proto?rev=r9109088#L130
	SpeechkitName() string

	// MarshalJSONPayload should return exact speechkit directive json representation
	// For capability directives it holds all directive fields except Name.
	// For other directives the representation should be checked against clients.
	MarshalJSONPayload() ([]byte, error)
}

func newSpeechkitDirective(directive SpeechkitDirective) (*speechkitpb.TDirective, error) {
	payloadBytes, err := directive.MarshalJSONPayload()
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal %s directive payload to json: %w", directive.SpeechkitName(), err)
	}

	var payload structpb.Struct
	if err := payload.UnmarshalJSON(payloadBytes); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal %s directive payload from json: %w", directive.SpeechkitName(), err)
	}

	return &speechkitpb.TDirective{
		Type:          "client_action",
		Name:          directive.SpeechkitName(),
		AnalyticsType: directive.SpeechkitName(),
		Payload:       &payload,
		IsLedSilent:   false,
		EndpointId:    wrapperspb.String(directive.EndpointID()),
	}, nil
}

// marshalSpeechkitDirective returns serialized NAlice.NSpeechKit.TDirective from alice/megamind/protos/speechkit/directives.proto
func marshalSpeechkitDirective(directive SpeechkitDirective) ([]byte, error) {
	speechkitDirective, err := newSpeechkitDirective(directive)
	if err != nil {
		return nil, xerrors.Errorf("failed to create speechkit directive: %w", err)
	}
	rawSpeechKitDirective, err := proto.Marshal(speechkitDirective)
	if err != nil {
		return nil, xerrors.Errorf("proto marshal error: %w", err)
	}
	return rawSpeechKitDirective, nil
}
