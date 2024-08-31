package tuya

import (
	"encoding/json"
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var colorSceneNumToSceneID map[int]model.ColorSceneID

type ColorScene struct {
	SceneNum   int              `json:"scene_num"`
	SceneUnits []ColorSceneUnit `json:"scene_units"`
}

type ColorSceneUnitChangeMode string

type ColorSceneUnit struct {
	UnitChangeMode       ColorSceneUnitChangeMode `json:"unit_change_mode"`
	UnitSwitchDuration   uint32                   `json:"unit_switch_duration"`   // 1-100, where 1 is the slowest and 100 is the fastest
	UnitGradientDuration uint32                   `json:"unit_gradient_duration"` // 1-100, where 1 is the slowest and 100 is the fastest
	H                    uint32                   `json:"h"`                      // 0-360
	S                    uint32                   `json:"s"`                      // 0-1000
	V                    uint32                   `json:"v"`                      // 0-1000
	Bright               uint32                   `json:"bright"`                 // 0-1000
	Temperature          uint32                   `json:"temperature"`            // 0-1000
}

func GetColorSceneParameters(lampScenePool ScenePoolID) *model.ColorSceneParameters {
	colorScenes := GetColorScenes(lampScenePool)
	scenes := make(model.ColorScenes, 0, len(colorScenes))
	for colorSceneID := range colorScenes {
		scenes = append(scenes, model.ColorScene{ID: colorSceneID})
	}
	sort.Sort(model.ColorSceneSorting(scenes))
	return &model.ColorSceneParameters{Scenes: scenes}
}

func GetColorScenes(lampScenePool ScenePoolID) map[model.ColorSceneID]ColorScene {
	copiedMap := make(map[model.ColorSceneID]ColorScene)
	for id, scene := range knownScenePools[lampScenePool] {
		copiedMap[id] = scene
	}
	return copiedMap
}

type SceneDataState struct {
	SceneNum int `json:"scene_num"`
}

func (sds *SceneDataState) FromTuyaCommandValue(value interface{}) error {
	strValue, ok := value.(string)
	if !ok {
		return xerrors.New("failed to convert scene_data value to scene data state: value is not string")
	}
	return json.Unmarshal([]byte(strValue), &sds)
}

func (sds SceneDataState) ColorSceneID() (model.ColorSceneID, bool) {
	colorSceneID, exists := colorSceneNumToSceneID[sds.SceneNum]
	return colorSceneID, exists
}
