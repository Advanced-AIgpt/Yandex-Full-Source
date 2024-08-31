package discovery

import (
	"fmt"
	"math/rand"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sup"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
)

func getNewDevicesPushInfo(devices []DeviceDiffInfo, skillInfo provider.SkillInfo, link string) sup.PushInfo {
	var text string
	switch len(devices) {
	case 0:
		// do nothing
	case 1:
		var constructedTexts []string
		if skillInfo.SkillID == model.YANDEXIO {
			constructedTexts = []string{
				"Пиу-пиу! Обнаружила новое устройство.",
				"Вижу, у вас появилось новое устройство",
				"О, у вас появилось новое устройство. Здорово.",
				"Ку! Я вижу новое устройство.",
				"Птичка на хвосте принесла, что у вас есть новое устройство.",
				"Ой, у вас появилось новое устройство? Как я рада.",
				"Что я вижу! У вас появилось новое устройство.",
				"Вижу, вы приобрели новое устройство. Это новый этап наших отношений.",
			}
		} else {
			constructedTexts = []string{
				fmt.Sprintf("Пиу-пиу! Обнаружено новое устройство от %s.", skillInfo.HumanReadableName),
				fmt.Sprintf("Вижу, у вас появилось новое устройство от %s.", skillInfo.HumanReadableName),
				fmt.Sprintf("О, у вас появилось новое устройство от %s. Здорово.", skillInfo.HumanReadableName),
				fmt.Sprintf("Ку! Я вижу новое устройство от %s.", skillInfo.HumanReadableName),
				fmt.Sprintf("Птичка на хвосте принесла, что у вас есть новое устройство от %s.", skillInfo.HumanReadableName),
				fmt.Sprintf("Ой, у вас появилось новое устройство от %s? Как я рада.", skillInfo.HumanReadableName),
				fmt.Sprintf("Что я вижу! У вас появилось новое устройство от %s.", skillInfo.HumanReadableName),
				fmt.Sprintf("Вижу, вы приобрели новое устройство от %s. Это новый этап наших отношений.", skillInfo.HumanReadableName),
			}
		}

		text = constructedTexts[rand.Intn(len(constructedTexts))]
	default:
		var constructedTexts []string
		if skillInfo.SkillID == model.YANDEXIO {
			constructedTexts = []string{
				"Пиу-пиу! Обнаружены новые устройства.",
				"Вижу, у вас появились новые устройства.",
				"О, у вас появились новые устройства. Здорово.",
				"Ку! Я вижу новые устройства.",
				"Птичка на хвосте принесла, что у вас есть новые устройства.",
				"Ой, у вас появились новые устройства? Как я рада.",
				"Что я вижу! У вас появились новые устройства.",
				"Вижу, вы приобрели новые устройства. Это новый этап наших отношений.",
			}
		} else {
			constructedTexts = []string{
				fmt.Sprintf("Пиу-пиу! Обнаружены новые устройства от %s.", skillInfo.HumanReadableName),
				fmt.Sprintf("Вижу, у вас появились новые устройства от %s.", skillInfo.HumanReadableName),
				fmt.Sprintf("О, у вас появились новые устройства от %s. Здорово.", skillInfo.HumanReadableName),
				fmt.Sprintf("Ку! Я вижу новые устройства от %s.", skillInfo.HumanReadableName),
				fmt.Sprintf("Птичка на хвосте принесла, что у вас есть новые устройства от %s.", skillInfo.HumanReadableName),
				fmt.Sprintf("Ой, у вас появились новые устройства от %s? Как я рада.", skillInfo.HumanReadableName),
				fmt.Sprintf("Что я вижу! У вас появились новые устройства от %s.", skillInfo.HumanReadableName),
				fmt.Sprintf("Вижу, вы приобрели новые устройства от %s. Это новый этап наших отношений.", skillInfo.HumanReadableName),
			}
		}
		text = constructedTexts[rand.Intn(len(constructedTexts))]
	}
	return sup.PushInfo{
		ID:               sup.NewDevicesPushID,
		Text:             text,
		Link:             link,
		ThrottlePolicyID: sup.NewDevicesThrottlePolicy,
	}
}
