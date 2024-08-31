package libmegamind

import (
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
)

func RunRequestLogView(key string, runRequest *scenarios.TScenarioRunRequest) log.Field {
	dataSources := map[int32]string{}

	for dataSourceID := range runRequest.GetDataSources() {
		dataSources[dataSourceID] = common.EDataSourceType_name[dataSourceID]
	}
	payload := map[string]interface{}{
		"platform":        runRequest.GetBaseRequest().GetClientInfo().GetPlatform(),
		"app_id":          runRequest.GetBaseRequest().GetClientInfo().GetAppId(),
		"text_utterance":  runRequest.GetInput().GetText().GetUtterance(),
		"voice_utterance": runRequest.GetInput().GetVoice().GetUtterance(),
		"data_sources":    dataSources,
	}

	return log.Any(key, payload)
}

func ApplyRequestLogView(key string, applyRequest *scenarios.TScenarioApplyRequest) log.Field {
	dataSources := map[int32]string{}

	for dataSourceID := range applyRequest.GetDataSources() {
		dataSources[dataSourceID] = common.EDataSourceType_name[dataSourceID]
	}
	payload := map[string]interface{}{
		"platform":        applyRequest.GetBaseRequest().GetClientInfo().GetPlatform(),
		"app_id":          applyRequest.GetBaseRequest().GetClientInfo().GetAppId(),
		"text_utterance":  applyRequest.GetInput().GetText().GetUtterance(),
		"voice_utterance": applyRequest.GetInput().GetVoice().GetUtterance(),
		"data_sources":    dataSources,
	}

	return log.Any(key, payload)
}
