package processors

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	videoitem "a.yandex-team.ru/alice/protos/data/video"
)

type VideoPlayDirective struct {
	URL         string
	Name        string
	Description string
	Protocol    model.VideoStreamProtocol
}

func (v VideoPlayDirective) ToDirective() *scenarios.TVideoPlayDirective {
	return &scenarios.TVideoPlayDirective{
		Uri: v.URL,
		Item: &videoitem.TVideoItem{
			PlayUri:          v.URL,
			Name:             v.Name,
			Description:      v.Description,
			Type:             "camera_stream",
			ProviderName:     "camera_stream",
			CameraStreamInfo: &videoitem.TVideoItem_TCameraStreamInfo{Type: string(v.Protocol)},
			ProviderItemId:   "camera_stream",
		},
	}
}
