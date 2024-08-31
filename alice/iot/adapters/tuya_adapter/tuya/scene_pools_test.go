package tuya

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/slices"
)

func TestColorSceneIDUnique(t *testing.T) {
	for _, scenePool := range knownScenePools {
		sceneNums := make(map[int]bool)
		for _, scene := range scenePool {
			assert.False(t, sceneNums[scene.SceneNum], "scene num must be unique between the scenes")
			sceneNums[scene.SceneNum] = true
		}
		assert.Equal(t, len(colorSceneNumToSceneID), len(scenePool))
	}
}

func TestSceneNumIsConsistentBetweenScenePools(t *testing.T) {
	sceneIDtoNums := make(map[model.ColorSceneID][]int)
	for _, scenePool := range knownScenePools {
		for sceneID, scene := range scenePool {
			sceneIDtoNums[sceneID] = append(sceneIDtoNums[sceneID], scene.SceneNum)
		}
	}
	for sceneID, sceneNums := range sceneIDtoNums {
		sceneNums = slices.Dedup(sceneNums)
		assert.Len(t, sceneNums, 1, "scene %s has distinct scene num between scene pools", sceneID)
	}
}

func TestTuyaScenePoolsValidity(t *testing.T) {
	for scenePoolID, scenePool := range knownScenePools {
		if scenePoolID != TuyaScenePool && scenePoolID != TuyaV2ScenePool {
			continue
		}
		for _, scene := range scenePool {
			assert.LessOrEqual(t, len(scene.SceneUnits), 8) // tuya chip supports only 8 scene units
			hasStaticSceneUnits := false
			for _, sceneUnit := range scene.SceneUnits {
				if sceneUnit.UnitChangeMode == StaticColorSceneUnitChangeMode {
					hasStaticSceneUnits = true
					break
				}
			}
			if hasStaticSceneUnits {
				assert.Len(t, scene.SceneUnits, 1) // static change mode on tuya chip scene do not support more than 1 scene unit
			}
		}
	}
}
