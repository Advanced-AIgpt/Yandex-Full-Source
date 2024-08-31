package dialoglib

import (
	"fmt"
	"log"
	"math/rand"

	"strings"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

func IsYandexStation(meta *sdk.Meta) bool {
	return !meta.Interfaces.Screen || strings.HasPrefix(meta.ClientID, "ru.yandex.iosdk.elariwatch")
}

func IsDeviceAllowedForPromo(meta *sdk.Meta) bool {
	return !(!meta.Interfaces.Screen || strings.Contains(meta.ClientID, "ru.yandex.iosdk.elariwatch") ||
		strings.Contains(meta.ClientID, "ru.yandex.mobile.navigator") || strings.Contains(
		meta.ClientID, "yandex.auto") || strings.Contains(meta.ClientID, "yandexnavi"))
}

type RepliesManager struct {
	random         rand.Rand
	replyTemplates map[string]map[string][]CueTemplate
}

func CreateRepliesManager(random rand.Rand, replyTemplates map[string]map[string][]CueTemplate) RepliesManager {
	for state, replies := range replyTemplates {
		for phrase, cueTemplates := range replies {
			for i := range cueTemplates {
				cueTemplates[i].templateName = fmt.Sprintf("s{%s}.p{%s}.i{%d}", state, phrase, i)
				if err := cueTemplates[i].Compile(); err != nil {
					log.Panicf("invalid cue template %s to compile: %#v", cueTemplates[i].templateName, err)
				}
			}
		}
	}
	return RepliesManager{
		random:         random,
		replyTemplates: replyTemplates,
	}
}

func (manager *RepliesManager) ChooseCueTemplate(state, phrase string) *CueTemplate {
	cueTemplates := manager.replyTemplates[state][phrase]
	if len(cueTemplates) == 0 {
		return &CueTemplate{templateName: fmt.Sprintf("s{%s}.p{%s}", state, phrase)}
	}
	return &cueTemplates[manager.random.Intn(len(cueTemplates))]
}
