package handlers

import (
	"context"
	"fmt"
	"net/http"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/gamma/server/skills"
	api "a.yandex-team.ru/alice/gamma/server/webhook/api/admin"
)

func TestGetTestSkillResponse(t *testing.T) {
	requests := []api.Request{
		{SkillIds: []string{"echo"}},
		{SkillIds: []string{"starwars", "other", "stuff"}},
		{SkillIds: []string{"parking", "meters"}},
	}

	skillsMap := map[string]skills.Info{
		"echo":     {ID: "echo"},
		"starwars": {ID: "skywalker"},
	}

	factory := skills.InMemoryProviderFactory{SkillsMap: skillsMap}
	provider_, _ := factory.CreateProvider()
	admin := CreateAdmin(provider_)

	response := admin.GetTestSkill(context.Background(), requests, func(skillId string) string {
		return fmt.Sprintf("http://galecore/skill/%s", skillId)
	})

	expectedResponse := []interface{}{
		api.SuccessResponse{
			ResponseBody: api.SuccessBody{
				BackendSettings:     api.BackendSettings{URI: "http://galecore/skill/echo"},
				ID:                  "echo",
				Salt:                "gammasaltecho",
				Logo:                api.Logo{AvatarID: ""},
				Name:                "Gamma Skill Test",
				StoreURL:            "",
				OnAir:               true,
				ExposeInternalFlags: true,
			},
		},
		api.SuccessResponse{
			ResponseBody: api.SuccessBody{
				BackendSettings:     api.BackendSettings{URI: "http://galecore/skill/starwars"},
				ID:                  "starwars",
				Salt:                "gammasaltstarwars",
				Logo:                api.Logo{AvatarID: ""},
				Name:                "Gamma Skill Test",
				StoreURL:            "",
				OnAir:               true,
				ExposeInternalFlags: true,
			},
		},
		api.ErrorResponse{
			ResponseBody: api.ErrorBody{
				Code:    http.StatusNotFound,
				Message: "Skill not registered",
				SkillID: "parking",
			},
		},
	}
	assert.Equal(t, expectedResponse, response, "Responses should be equal")
}
