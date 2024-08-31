package sharing

import (
	"encoding/json"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/oauth"
)

type TokenType string

const (
	OAuthType TokenType = "OAuthToken"
	XCodeType TokenType = "XToken"
)

type UserType string

const (
	OwnerType UserType = "OWNER"
	GuestType UserType = "GUEST"
)

func tokenTypeForDirective(tokenType oauth.TokenType) TokenType {
	if tokenType == oauth.XTokenTokenType {
		return XCodeType
	} else {
		return OAuthType
	}
}

// AddAccountSKDirective is used for adding guest voiceprint to shared device
// see https://st.yandex-team.ru/IOT-1280#61fbfb20675fcd07f3fd9ca2 for details
type AddAccountSKDirective struct {
	endpointID string

	EncryptedCode       string    `json:"encrypted_code,omitempty"`
	EncryptedSessionKey string    `json:"encrypted_session_key,omitempty"`
	Signature           string    `json:"signature,omitempty"`
	TokenType           TokenType `json:"token_type,omitempty"`
	UserType            UserType  `json:"user_type"`
}

func (a AddAccountSKDirective) EndpointID() string {
	return a.endpointID
}

func (a AddAccountSKDirective) SpeechkitName() string {
	return "multiaccount_add_account"
}

func (a *AddAccountSKDirective) MarshalJSONPayload() ([]byte, error) {
	return json.Marshal(a)
}

// RemoveAccountSKDirective is used for removing guest voiceprint from shared device
type RemoveAccountSKDirective struct {
	endpointID string
	PUID       uint64 `json:"puid"`
}

func (r RemoveAccountSKDirective) EndpointID() string {
	return r.endpointID
}

func (r RemoveAccountSKDirective) SpeechkitName() string {
	return "multiaccount_remove_account"
}

func (r *RemoveAccountSKDirective) MarshalJSONPayload() ([]byte, error) {
	return json.Marshal(r)
}

// BioStartSoundEnrollmentDirective is used for adding voiceprint onto specific devices that acquire tokens for voiceprint via sound (mini 1)
// see https://st.yandex-team.ru/SK-6061 for details
type BioStartSoundEnrollmentSKDirective struct {
	endpointID string
}

func (r BioStartSoundEnrollmentSKDirective) EndpointID() string {
	return r.endpointID
}

func (r BioStartSoundEnrollmentSKDirective) SpeechkitName() string {
	return "bio_start_sound_enrollment"
}

//nolint:SA9005
func (r *BioStartSoundEnrollmentSKDirective) MarshalJSONPayload() ([]byte, error) {
	return json.Marshal(r)
}
