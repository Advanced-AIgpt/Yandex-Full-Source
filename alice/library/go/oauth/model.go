package oauth

import (
	"fmt"
	"strconv"
)

type AuthCodeStrength string

const (
	MediumAuthCodeStrength AuthCodeStrength = "medium" // 8 lowercase letters and digits in xcode
	LongAuthCodeStrength   AuthCodeStrength = "long"   // 16 lowercase letters and digits in xcode
)

type UserData struct {
	UserTicket string
	IP         string
}

type IssueAuthorizationCodeParams struct {
	ClientID          string
	ClientSecret      string
	CodeStrength      AuthCodeStrength
	TTL               *int
	RequireActivation *bool
}

func (i IssueAuthorizationCodeParams) toFormData(consumer string) map[string]string {
	formData := map[string]string{
		"consumer":      consumer,
		"client_id":     i.ClientID,
		"client_secret": i.ClientSecret,
		"code_strength": string(i.CodeStrength),
	}

	if i.TTL != nil {
		formData["ttl"] = strconv.Itoa(*i.TTL)
	}

	if i.RequireActivation != nil {
		formData["require_activation"] = fmt.Sprintf("%t", *i.RequireActivation)
	}

	return formData
}

// IssueAuthorizationCodeResponse contains response with x-code
// see https://wiki.yandex-team.ru/oauth/api/#issueauthorizationcode for more details
type IssueAuthorizationCodeResponse struct {
	Status    string   `json:"status"`
	Code      string   `json:"code"` // actual x-code
	ExpiresIn int      `json:"expires_in"`
	Errors    []string `json:"errors"`
}

type AuthorizationCode struct {
	Code      string
	ExpiresIn int
}
