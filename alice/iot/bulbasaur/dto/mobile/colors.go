package mobile

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

var mobileColorsMap = map[model.ColorID]model.HSV{
	model.ColorIDRed:       {H: 0, S: 65, V: 100},
	model.ColorIDCoral:     {H: 8, S: 55, V: 98},
	model.ColorIDOrange:    {H: 25, S: 70, V: 100},
	model.ColorIDYellow:    {H: 40, S: 70, V: 100},
	model.ColorIDLime:      {H: 73, S: 96, V: 100},
	model.ColorIDGreen:     {H: 120, S: 55, V: 90},
	model.ColorIDEmerald:   {H: 160, S: 80, V: 90},
	model.ColorIDTurquoise: {H: 180, S: 80, V: 90},
	model.ColorIDCyan:      {H: 190, S: 60, V: 100},
	model.ColorIDBlue:      {H: 225, S: 55, V: 90},
	model.ColorIDLavender:  {H: 255, S: 55, V: 90},
	model.ColorIDViolet:    {H: 270, S: 55, V: 90},
	model.ColorIDPurple:    {H: 300, S: 70, V: 90},
	model.ColorIDOrchid:    {H: 305, S: 50, V: 90},
	model.ColorIDMauve:     {H: 340, S: 45, V: 90},
	model.ColorIDRaspberry: {H: 345, S: 70, V: 90},
}

type IColorSettingStateView interface {
	isColorSettingStateView()
}

type ColorStateView struct {
	ID    model.ColorID   `json:"id"`
	Name  string          `json:"name"`
	Type  model.ColorType `json:"type"`
	Value model.HSV       `json:"value"`
}

func (c ColorStateView) isColorSettingStateView() {}

func (c *ColorStateView) FromColor(mc model.Color) {
	c.ID = mc.ID
	c.Name = mc.Name
	c.Type = mc.Type

	c.Value = mc.ValueHSV
	if value, ok := mobileColorsMap[mc.ID]; ok {
		c.Value = value
	}
}

func ColorStateViewList(colors []model.Color) []ColorStateView {
	var csvl []ColorStateView
	for _, c := range colors {
		mobileColors := ColorStateView{}
		mobileColors.FromColor(c)
		csvl = append(csvl, mobileColors)
	}
	return csvl
}

type ColorSceneView struct {
	ID   model.ColorSceneID `json:"id"`
	Name string             `json:"name"`
}

func (c ColorSceneView) isColorSettingStateView() {}

func (c *ColorSceneView) FromColorScene(cs model.ColorScene) {
	c.ID = cs.ID
	c.Name = cs.Name
	if len(c.Name) == 0 {
		if scene, ok := model.KnownColorScenes[c.ID]; ok {
			c.Name = scene.Name
		}
	}
}

func (c *ColorSceneView) FromColorSceneID(id model.ColorSceneID) {
	c.ID = id
	c.Name = "Сцена"
	if scene, ok := model.KnownColorScenes[c.ID]; ok {
		c.Name = scene.Name
	}
}
