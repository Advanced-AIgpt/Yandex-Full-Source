package tuya

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

var ProductionColorPalette = model.ColorPaletteType{
	model.ColorIDRed:       {ID: model.ColorIDRed, Name: "Красный", Type: model.Multicolor, ValueHSV: model.HSV{H: 0, S: 1000, V: 1000}},
	model.ColorIDCoral:     {ID: model.ColorIDCoral, Name: "Коралловый", Type: model.Multicolor, ValueHSV: model.HSV{H: 20, S: 990, V: 1000}},
	model.ColorIDOrange:    {ID: model.ColorIDOrange, Name: "Оранжевый", Type: model.Multicolor, ValueHSV: model.HSV{H: 14, S: 1000, V: 1000}},
	model.ColorIDYellow:    {ID: model.ColorIDYellow, Name: "Желтый", Type: model.Multicolor, ValueHSV: model.HSV{H: 28, S: 1000, V: 1000}},
	model.ColorIDLime:      {ID: model.ColorIDLime, Name: "Салатовый", Type: model.Multicolor, ValueHSV: model.HSV{H: 52, S: 1000, V: 1000}},
	model.ColorIDGreen:     {ID: model.ColorIDGreen, Name: "Зеленый", Type: model.Multicolor, ValueHSV: model.HSV{H: 95, S: 1000, V: 1000}},
	model.ColorIDEmerald:   {ID: model.ColorIDEmerald, Name: "Изумрудный", Type: model.Multicolor, ValueHSV: model.HSV{H: 130, S: 1000, V: 1000}},
	model.ColorIDTurquoise: {ID: model.ColorIDTurquoise, Name: "Бирюзовый", Type: model.Multicolor, ValueHSV: model.HSV{H: 162, S: 1000, V: 1000}},
	model.ColorIDCyan:      {ID: model.ColorIDCyan, Name: "Голубой", Type: model.Multicolor, ValueHSV: model.HSV{H: 190, S: 1000, V: 1000}},
	model.ColorIDBlue:      {ID: model.ColorIDBlue, Name: "Синий", Type: model.Multicolor, ValueHSV: model.HSV{H: 240, S: 1000, V: 1000}},
	model.ColorIDMoonlight: {ID: model.ColorIDMoonlight, Name: "Лунный", Type: model.Multicolor, ValueHSV: model.HSV{H: 231, S: 1000, V: 1000}},
	model.ColorIDLavender:  {ID: model.ColorIDLavender, Name: "Сиреневый", Type: model.Multicolor, ValueHSV: model.HSV{H: 270, S: 1000, V: 1000}},
	model.ColorIDViolet:    {ID: model.ColorIDViolet, Name: "Фиолетовый", Type: model.Multicolor, ValueHSV: model.HSV{H: 280, S: 1000, V: 1000}},
	model.ColorIDPurple:    {ID: model.ColorIDPurple, Name: "Пурпурный", Type: model.Multicolor, ValueHSV: model.HSV{H: 340, S: 820, V: 1000}},
	model.ColorIDOrchid:    {ID: model.ColorIDOrchid, Name: "Орхидея", Type: model.Multicolor, ValueHSV: model.HSV{H: 335, S: 750, V: 1000}},
	model.ColorIDRaspberry: {ID: model.ColorIDRaspberry, Name: "Малина", Type: model.Multicolor, ValueHSV: model.HSV{H: 358, S: 980, V: 1000}},
	model.ColorIDMauve:     {ID: model.ColorIDMauve, Name: "Лиловый", Type: model.Multicolor, ValueHSV: model.HSV{H: 10, S: 780, V: 1000}},
}
var E27Lamp2ColorPalette = model.ColorPaletteType{
	model.ColorIDRed:       {ID: model.ColorIDRed, Name: "Красный", Type: model.Multicolor, ValueHSV: model.HSV{H: 0, S: 1000, V: 1000}},
	model.ColorIDCoral:     {ID: model.ColorIDCoral, Name: "Коралловый", Type: model.Multicolor, ValueHSV: model.HSV{H: 20, S: 1000, V: 1000}},
	model.ColorIDOrange:    {ID: model.ColorIDOrange, Name: "Оранжевый", Type: model.Multicolor, ValueHSV: model.HSV{H: 28, S: 950, V: 1000}},
	model.ColorIDYellow:    {ID: model.ColorIDYellow, Name: "Желтый", Type: model.Multicolor, ValueHSV: model.HSV{H: 40, S: 840, V: 1000}},
	model.ColorIDLime:      {ID: model.ColorIDLime, Name: "Салатовый", Type: model.Multicolor, ValueHSV: model.HSV{H: 52, S: 1000, V: 1000}},
	model.ColorIDGreen:     {ID: model.ColorIDGreen, Name: "Зеленый", Type: model.Multicolor, ValueHSV: model.HSV{H: 95, S: 1000, V: 1000}},
	model.ColorIDEmerald:   {ID: model.ColorIDEmerald, Name: "Изумрудный", Type: model.Multicolor, ValueHSV: model.HSV{H: 130, S: 1000, V: 1000}},
	model.ColorIDTurquoise: {ID: model.ColorIDTurquoise, Name: "Бирюзовый", Type: model.Multicolor, ValueHSV: model.HSV{H: 162, S: 1000, V: 1000}},
	model.ColorIDCyan:      {ID: model.ColorIDCyan, Name: "Голубой", Type: model.Multicolor, ValueHSV: model.HSV{H: 190, S: 1000, V: 1000}},
	model.ColorIDBlue:      {ID: model.ColorIDBlue, Name: "Синий", Type: model.Multicolor, ValueHSV: model.HSV{H: 240, S: 1000, V: 1000}},
	model.ColorIDMoonlight: {ID: model.ColorIDMoonlight, Name: "Лунный", Type: model.Multicolor, ValueHSV: model.HSV{H: 231, S: 100, V: 1000}},
	model.ColorIDLavender:  {ID: model.ColorIDLavender, Name: "Сиреневый", Type: model.Multicolor, ValueHSV: model.HSV{H: 270, S: 1000, V: 1000}},
	model.ColorIDViolet:    {ID: model.ColorIDViolet, Name: "Фиолетовый", Type: model.Multicolor, ValueHSV: model.HSV{H: 280, S: 1000, V: 1000}},
	model.ColorIDPurple:    {ID: model.ColorIDPurple, Name: "Пурпурный", Type: model.Multicolor, ValueHSV: model.HSV{H: 340, S: 820, V: 1000}},
	model.ColorIDOrchid:    {ID: model.ColorIDOrchid, Name: "Орхидея", Type: model.Multicolor, ValueHSV: model.HSV{H: 335, S: 750, V: 1000}},
	model.ColorIDRaspberry: {ID: model.ColorIDRaspberry, Name: "Малина", Type: model.Multicolor, ValueHSV: model.HSV{H: 342, S: 980, V: 1000}},
	model.ColorIDMauve:     {ID: model.ColorIDMauve, Name: "Лиловый", Type: model.Multicolor, ValueHSV: model.HSV{H: 10, S: 780, V: 1000}},
}
var E27Lamp3ColorPalette = model.ColorPaletteType{
	model.ColorIDRed:       {ID: model.ColorIDRed, Name: "Красный", Type: model.Multicolor, ValueHSV: model.HSV{H: 0, S: 1000, V: 1000}},
	model.ColorIDCoral:     {ID: model.ColorIDCoral, Name: "Коралловый", Type: model.Multicolor, ValueHSV: model.HSV{H: 20, S: 1000, V: 1000}},
	model.ColorIDOrange:    {ID: model.ColorIDOrange, Name: "Оранжевый", Type: model.Multicolor, ValueHSV: model.HSV{H: 32, S: 980, V: 1000}},
	model.ColorIDYellow:    {ID: model.ColorIDYellow, Name: "Желтый", Type: model.Multicolor, ValueHSV: model.HSV{H: 44, S: 1000, V: 1000}},
	model.ColorIDLime:      {ID: model.ColorIDLime, Name: "Салатовый", Type: model.Multicolor, ValueHSV: model.HSV{H: 52, S: 1000, V: 1000}},
	model.ColorIDGreen:     {ID: model.ColorIDGreen, Name: "Зеленый", Type: model.Multicolor, ValueHSV: model.HSV{H: 132, S: 980, V: 1000}},
	model.ColorIDEmerald:   {ID: model.ColorIDEmerald, Name: "Изумрудный", Type: model.Multicolor, ValueHSV: model.HSV{H: 130, S: 1000, V: 1000}},
	model.ColorIDTurquoise: {ID: model.ColorIDTurquoise, Name: "Бирюзовый", Type: model.Multicolor, ValueHSV: model.HSV{H: 162, S: 1000, V: 1000}},
	model.ColorIDCyan:      {ID: model.ColorIDCyan, Name: "Голубой", Type: model.Multicolor, ValueHSV: model.HSV{H: 190, S: 1000, V: 1000}},
	model.ColorIDBlue:      {ID: model.ColorIDBlue, Name: "Синий", Type: model.Multicolor, ValueHSV: model.HSV{H: 240, S: 1000, V: 1000}},
	model.ColorIDMoonlight: {ID: model.ColorIDMoonlight, Name: "Лунный", Type: model.Multicolor, ValueHSV: model.HSV{H: 231, S: 100, V: 1000}},
	model.ColorIDLavender:  {ID: model.ColorIDLavender, Name: "Сиреневый", Type: model.Multicolor, ValueHSV: model.HSV{H: 267, S: 950, V: 1000}},
	model.ColorIDViolet:    {ID: model.ColorIDViolet, Name: "Фиолетовый", Type: model.Multicolor, ValueHSV: model.HSV{H: 275, S: 880, V: 1000}},
	model.ColorIDPurple:    {ID: model.ColorIDPurple, Name: "Пурпурный", Type: model.Multicolor, ValueHSV: model.HSV{H: 305, S: 860, V: 1000}},
	model.ColorIDOrchid:    {ID: model.ColorIDOrchid, Name: "Орхидея", Type: model.Multicolor, ValueHSV: model.HSV{H: 335, S: 740, V: 1000}},
	model.ColorIDRaspberry: {ID: model.ColorIDRaspberry, Name: "Малина", Type: model.Multicolor, ValueHSV: model.HSV{H: 342, S: 980, V: 1000}},
	model.ColorIDMauve:     {ID: model.ColorIDMauve, Name: "Лиловый", Type: model.Multicolor, ValueHSV: model.HSV{H: 10, S: 780, V: 1000}},
}
var TuyaLampColorPalette = model.ColorPaletteType{
	model.ColorIDRed:       {ID: model.ColorIDRed, Name: "Красный", Type: model.Multicolor, ValueHSV: model.HSV{H: 0, S: 1000, V: 1000}},
	model.ColorIDCoral:     {ID: model.ColorIDCoral, Name: "Коралловый", Type: model.Multicolor, ValueHSV: model.HSV{H: 3, S: 980, V: 1000}},
	model.ColorIDOrange:    {ID: model.ColorIDOrange, Name: "Оранжевый", Type: model.Multicolor, ValueHSV: model.HSV{H: 11, S: 990, V: 1000}},
	model.ColorIDYellow:    {ID: model.ColorIDYellow, Name: "Желтый", Type: model.Multicolor, ValueHSV: model.HSV{H: 17, S: 990, V: 1000}},
	model.ColorIDLime:      {ID: model.ColorIDLime, Name: "Салатовый", Type: model.Multicolor, ValueHSV: model.HSV{H: 54, S: 1000, V: 1000}},
	model.ColorIDGreen:     {ID: model.ColorIDGreen, Name: "Зеленый", Type: model.Multicolor, ValueHSV: model.HSV{H: 105, S: 980, V: 1000}},
	model.ColorIDEmerald:   {ID: model.ColorIDEmerald, Name: "Изумрудный", Type: model.Multicolor, ValueHSV: model.HSV{H: 135, S: 880, V: 1000}},
	model.ColorIDTurquoise: {ID: model.ColorIDTurquoise, Name: "Бирюзовый", Type: model.Multicolor, ValueHSV: model.HSV{H: 168, S: 940, V: 1000}},
	model.ColorIDCyan:      {ID: model.ColorIDCyan, Name: "Голубой", Type: model.Multicolor, ValueHSV: model.HSV{H: 198, S: 800, V: 1000}},
	model.ColorIDBlue:      {ID: model.ColorIDBlue, Name: "Синий", Type: model.Multicolor, ValueHSV: model.HSV{H: 228, S: 1000, V: 800}},
	model.ColorIDMoonlight: {ID: model.ColorIDMoonlight, Name: "Лунный", Type: model.Multicolor, ValueHSV: model.HSV{H: 350, S: 350, V: 1000}},
	model.ColorIDLavender:  {ID: model.ColorIDLavender, Name: "Сиреневый", Type: model.Multicolor, ValueHSV: model.HSV{H: 256, S: 860, V: 1000}},
	model.ColorIDViolet:    {ID: model.ColorIDViolet, Name: "Фиолетовый", Type: model.Multicolor, ValueHSV: model.HSV{H: 271, S: 900, V: 1000}},
	model.ColorIDPurple:    {ID: model.ColorIDPurple, Name: "Пурпурный", Type: model.Multicolor, ValueHSV: model.HSV{H: 334, S: 920, V: 1000}},
	model.ColorIDOrchid:    {ID: model.ColorIDOrchid, Name: "Орхидея", Type: model.Multicolor, ValueHSV: model.HSV{H: 350, S: 940, V: 1000}},
	model.ColorIDRaspberry: {ID: model.ColorIDRaspberry, Name: "Малина", Type: model.Multicolor, ValueHSV: model.HSV{H: 359, S: 960, V: 1000}},
	model.ColorIDMauve:     {ID: model.ColorIDMauve, Name: "Лиловый", Type: model.Multicolor, ValueHSV: model.HSV{H: 1, S: 970, V: 1000}},
}

func getColorPalette(pid TuyaDeviceProductID) model.ColorPaletteType {
	switch pid {
	case LampE27Lemon2YandexProductID:
		return E27Lamp2ColorPalette
	case LampE27Lemon3YandexProductID:
		return E27Lamp3ColorPalette
	case LampE14TestYandexProductID, LampE14Test2YandexProductID, LampE14MPYandexProductID, LampGU10TestYandexProductID, LampGU10MPYandexProductID:
		return TuyaLampColorPalette
	default:
		return ProductionColorPalette
	}
}

func getColorByHS(hsv model.HSV, pid TuyaDeviceProductID) (model.Color, bool) {
	for _, color := range getColorPalette(pid) {
		if color.ValueHSV.H == hsv.H && color.ValueHSV.S == hsv.S {
			return color, true
		}

	}
	return model.Color{}, false
}
