package admin

import (
	"net/http"

	"a.yandex-team.ru/library/go/core/xerrors"

	"a.yandex-team.ru/alice/gamma/server/skills"
)

type Logo struct {
	AvatarID string `json:"avatarId"`
}

type BackendSettings struct {
	URI string `json:"uri"`
}

type SuccessBody struct {
	BackendSettings     BackendSettings `json:"backendSettings"`
	ID                  string          `json:"id"`
	Salt                string          `json:"salt"`
	Logo                Logo            `json:"logo"`
	Name                string          `json:"name"`
	StoreURL            string          `json:"storeUrl"`
	OnAir               bool            `json:"onAir"`
	ExposeInternalFlags bool            `json:"exposeInternalFlags"`
}

type SuccessResponse struct {
	ResponseBody SuccessBody `json:"result"`
}

type ErrorBody struct {
	Code    int    `json:"code"`
	Message string `json:"message"`
	SkillID string `json:"skillId"`
}

type ErrorResponse struct {
	ResponseBody ErrorBody `json:"error"`
}

func newSuccessResponse(skillID, uri string) SuccessResponse {
	return SuccessResponse{
		ResponseBody: SuccessBody{
			BackendSettings:     BackendSettings{URI: uri},
			ID:                  skillID,
			Salt:                "gammasalt" + skillID,
			Logo:                Logo{AvatarID: ""},
			Name:                "Gamma Skill Test",
			StoreURL:            "",
			OnAir:               true,
			ExposeInternalFlags: true,
		},
	}
}

func newErrorResponse(code int, skillID, message string) ErrorResponse {
	return ErrorResponse{
		ResponseBody: ErrorBody{
			Code:    code,
			Message: message,
			SkillID: skillID,
		},
	}
}

func NewResponse(skillIDs []string, skillInfos []*skills.Info, errors []error, urlBuilder func(string) string) []interface{} {
	response := make([]interface{}, len(skillInfos))
	for i, skillInfo := range skillInfos {
		skillID := skillIDs[i]
		if errors[i] != nil {
			if xerrors.Is(errors[i], skills.NotFoundError) {
				response[i] = newErrorResponse(http.StatusNotFound, skillID, "Skill not registered")
			} else {
				response[i] = newErrorResponse(http.StatusInternalServerError, skillID, errors[i].Error())
			}
			continue
		}
		if skillInfo != nil {
			responseURL := urlBuilder(skillID)
			response[i] = newSuccessResponse(skillID, responseURL)
		}
	}
	return response
}
