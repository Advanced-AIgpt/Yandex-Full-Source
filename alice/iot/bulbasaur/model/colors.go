package model

import (
	"math"
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/megamind/protos/common"
)

var ColorPalette = ColorPaletteType{}
var ColorIDColorTemperatureMap = map[TemperatureK]ColorID{}
var ColorTemperatureColorIDMap = map[ColorID]TemperatureK{}
var ColorHSVColorIDMap = map[HSV]ColorID{}
var colorHSMap = map[HsTuple]ColorID{}
var colorRGBMap = map[RGB]ColorID{}

const (
	DefaultLeftBoundTemperatureK  uint64 = 2700
	DefaultRightBoundTemperatureK uint64 = 6500
)

var ColorTemperaturePaletteSorted = []TemperatureK{}

var ColorIDToAdditionalAliases = map[ColorID][]string{
	ColorIDOrchid:        {"Орхидея"},
	ColorIDWhite:         {"Обычный", "Нормальный"}, // нормальный! нормальный!!
	ColorIDRaspberry:     {"Малина"},
	ColorIDFieryWhite:    {"Огненный"},
	ColorIDMistyWhite:    {"Туманный"},
	ColorIDHeavenlyWhite: {"Небесный"},
}

func init() {
	ColorPalette = ColorPaletteType{
		ColorIDFieryWhite:    {ID: ColorIDFieryWhite, Name: "Огненный белый", Type: WhiteColor, ValueHSV: HSV{H: 25, S: 100, V: 100}, ValueRGB: 16739328, Temperature: TemperatureK(1500)},
		ColorIDSoftWhite:     {ID: ColorIDSoftWhite, Name: "Мягкий белый", Type: WhiteColor, ValueHSV: HSV{H: 32, S: 67, V: 100}, ValueRGB: 16756308, Temperature: TemperatureK(2700)},
		ColorIDWarmWhite:     {ID: ColorIDWarmWhite, Name: "Теплый белый", Type: WhiteColor, ValueHSV: HSV{H: 33, S: 49, V: 100}, ValueRGB: 16762498, Temperature: TemperatureK(3400)},
		ColorIDWhite:         {ID: ColorIDWhite, Name: "Белый", Type: WhiteColor, ValueHSV: HSV{H: 33, S: 28, V: 100}, ValueRGB: 16768952, Temperature: TemperatureK(4500)},
		ColorIDDaylight:      {ID: ColorIDDaylight, Name: "Дневной белый", Type: WhiteColor, ValueHSV: HSV{H: 27, S: 11, V: 100}, ValueRGB: 16773348, Temperature: TemperatureK(5600)},
		ColorIDColdWhite:     {ID: ColorIDColdWhite, Name: "Холодный белый", Type: WhiteColor, ValueHSV: HSV{H: 340, S: 2, V: 100}, ValueRGB: 16775932, Temperature: TemperatureK(6500)},
		ColorIDMistyWhite:    {ID: ColorIDMistyWhite, Name: "Туманный белый", Type: WhiteColor, ValueHSV: HSV{H: 228, S: 10, V: 100}, ValueRGB: 15133695, Temperature: TemperatureK(7500)},
		ColorIDHeavenlyWhite: {ID: ColorIDHeavenlyWhite, Name: "Небесный белый", Type: WhiteColor, ValueHSV: HSV{H: 225, S: 18, V: 100}, ValueRGB: 13819903, Temperature: TemperatureK(9000)},

		ColorIDRed:       {ID: ColorIDRed, Name: "Красный", Type: Multicolor, ValueHSV: HSV{H: 0, S: 96, V: 100}, ValueRGB: 16714250},
		ColorIDCoral:     {ID: ColorIDCoral, Name: "Коралловый", Type: Multicolor, ValueHSV: HSV{H: 6, S: 80, V: 100}, ValueRGB: 16729907},
		ColorIDOrange:    {ID: ColorIDOrange, Name: "Оранжевый", Type: Multicolor, ValueHSV: HSV{H: 14, S: 100, V: 100}, ValueRGB: 16727040},
		ColorIDYellow:    {ID: ColorIDYellow, Name: "Желтый", Type: Multicolor, ValueHSV: HSV{H: 25, S: 96, V: 100}, ValueRGB: 16740362},
		ColorIDLime:      {ID: ColorIDLime, Name: "Салатовый", Type: Multicolor, ValueHSV: HSV{H: 73, S: 96, V: 100}, ValueRGB: 13303562},
		ColorIDGreen:     {ID: ColorIDGreen, Name: "Зеленый", Type: Multicolor, ValueHSV: HSV{H: 135, S: 96, V: 100}, ValueRGB: 720711},
		ColorIDEmerald:   {ID: ColorIDEmerald, Name: "Изумрудный", Type: Multicolor, ValueHSV: HSV{H: 160, S: 96, V: 100}, ValueRGB: 720813},
		ColorIDTurquoise: {ID: ColorIDTurquoise, Name: "Бирюзовый", Type: Multicolor, ValueHSV: HSV{H: 177, S: 96, V: 100}, ValueRGB: 720883},
		ColorIDCyan:      {ID: ColorIDCyan, Name: "Голубой", Type: Multicolor, ValueHSV: HSV{H: 190, S: 96, V: 100}, ValueRGB: 710399},
		ColorIDBlue:      {ID: ColorIDBlue, Name: "Синий", Type: Multicolor, ValueHSV: HSV{H: 225, S: 96, V: 100}, ValueRGB: 673791},
		ColorIDMoonlight: {ID: ColorIDMoonlight, Name: "Лунный", Type: Multicolor, ValueHSV: HSV{H: 231, S: 10, V: 100}, ValueRGB: 15067647},
		ColorIDLavender:  {ID: ColorIDLavender, Name: "Сиреневый", Type: Multicolor, ValueHSV: HSV{H: 270, S: 96, V: 100}, ValueRGB: 8719103},
		ColorIDViolet:    {ID: ColorIDViolet, Name: "Фиолетовый", Type: Multicolor, ValueHSV: HSV{H: 280, S: 96, V: 100}, ValueRGB: 11340543},
		ColorIDPurple:    {ID: ColorIDPurple, Name: "Пурпурный", Type: Multicolor, ValueHSV: HSV{H: 306, S: 96, V: 100}, ValueRGB: 16714471},
		ColorIDOrchid:    {ID: ColorIDOrchid, Name: "Розовый", Type: Multicolor, ValueHSV: HSV{H: 325, S: 96, V: 100}, ValueRGB: 16714393},
		ColorIDMauve:     {ID: ColorIDMauve, Name: "Лиловый", Type: Multicolor, ValueHSV: HSV{H: 357, S: 83, V: 100}, ValueRGB: 16722742},
		ColorIDRaspberry: {ID: ColorIDRaspberry, Name: "Малиновый", Type: Multicolor, ValueHSV: HSV{H: 340, S: 100, V: 100}, ValueRGB: 16711765},
	}

	for _, v := range ColorPalette {
		if v.Type == WhiteColor {
			ColorIDColorTemperatureMap[v.Temperature] = v.ID
			ColorTemperatureColorIDMap[v.ID] = v.Temperature
			ColorTemperaturePaletteSorted = append(ColorTemperaturePaletteSorted, v.Temperature)
		}
		ColorHSVColorIDMap[v.ValueHSV] = v.ID
		colorHSMap[HsTuple{v.ValueHSV.H, v.ValueHSV.S}] = v.ID
		colorRGBMap[v.ValueRGB] = v.ID
	}
	sort.Slice(ColorTemperaturePaletteSorted, func(i, j int) bool {
		return ColorTemperaturePaletteSorted[i] < ColorTemperaturePaletteSorted[j]
	})
}

type ColorType string

const (
	Multicolor ColorType = "multicolor"
	WhiteColor ColorType = "white"
)

type RGB int

func (rgb RGB) isColorSettingValue() {}

func (rgb RGB) Clone() ColorSettingValue {
	return rgb
}

type HSV struct {
	H int `json:"h"`
	S int `json:"s"`
	V int `json:"v"`
}

func (hsv HSV) isColorSettingValue() {}

func (hsv HSV) Clone() ColorSettingValue {
	return HSV{
		H: hsv.H,
		S: hsv.S,
		V: hsv.V,
	}
}

type HsTuple struct {
	H int
	S int
}

type Color struct {
	ID          ColorID
	Name        string
	Type        ColorType
	ValueHSV    HSV
	ValueRGB    RGB
	Temperature TemperatureK
}

func (c *Color) ToColorSettingCapabilityState(instance ColorSettingCapabilityInstance) ColorSettingCapabilityState {
	state := ColorSettingCapabilityState{}

	//check color for temperature_k, cause temperature_k is a subset of rgb or hsv
	if temperature, exists := ColorTemperatureColorIDMap[c.ID]; exists {
		state.Instance = TemperatureKCapabilityInstance
		state.Value = temperature
		return state
	}

	switch instance {
	case TemperatureKCapabilityInstance:
		state.Instance = TemperatureKCapabilityInstance
		state.Value = c.Temperature
	case RgbColorCapabilityInstance:
		state.Instance = RgbColorCapabilityInstance
		state.Value = c.ValueRGB
	case HsvColorCapabilityInstance:
		state.Instance = HsvColorCapabilityInstance
		state.Value = HSV{
			H: c.ValueHSV.H,
			S: c.ValueHSV.S,
			V: c.ValueHSV.V,
		}
	}

	return state
}

type ColorPaletteType map[ColorID]Color

func (cp ColorPaletteType) toSlice() []Color {
	colors := make([]Color, 0, len(cp))
	for _, value := range cp {
		colors = append(colors, value)
	}
	return colors
}

func (cp ColorPaletteType) ToSortedSlice() []Color {
	colors := make([]Color, 0, len(cp))

	whites := cp.FilterType(WhiteColor).toSlice()
	sort.Slice(whites, func(i, j int) bool {
		return whites[i].Temperature < whites[j].Temperature
	})

	multies := cp.FilterType(Multicolor).toSlice()
	sort.Slice(multies, func(i, j int) bool {
		return multies[i].ValueHSV.H < multies[j].ValueHSV.H
	})

	colors = append(colors, whites...)
	colors = append(colors, multies...)

	return colors
}

func (cp ColorPaletteType) GetDefaultWhiteColor() Color {
	return cp["white"]
}

func (cp ColorPaletteType) GetColorByTemperatureK(temperatureK TemperatureK) Color {
	roundedTemperature := TemperatureK(math.Round(float64(temperatureK)/100) * 100)
	if colorID, exists := ColorIDColorTemperatureMap[roundedTemperature]; exists {
		if color, exists := cp[colorID]; exists {
			return color
		}
	}
	hsv := rgbToHsv(rgbToInt(temperatureToRgb(roundedTemperature)))
	return Color{
		Type:        WhiteColor,
		ValueHSV:    hsv,
		Temperature: roundedTemperature,
	}
}

func (cp ColorPaletteType) GetNearestColorByColorTemperature(temperatureK TemperatureK) Color {
	for i, paletteTemperature := range ColorTemperaturePaletteSorted {
		if temperatureK > paletteTemperature {
			continue
		}
		if i == 0 {
			return ColorPalette[ColorIDColorTemperatureMap[paletteTemperature]]
		}
		// we are in bounds of [i-1, i]
		leftDiff := temperatureK - ColorTemperaturePaletteSorted[i-1]
		rightDiff := paletteTemperature - temperatureK
		if leftDiff < rightDiff {
			return ColorPalette[ColorIDColorTemperatureMap[ColorTemperaturePaletteSorted[i-1]]]
		} else {
			return ColorPalette[ColorIDColorTemperatureMap[paletteTemperature]]
		}
	}
	lastTemperature := ColorTemperaturePaletteSorted[len(ColorTemperaturePaletteSorted)-1]
	return ColorPalette[ColorIDColorTemperatureMap[lastTemperature]]
}

func (cp ColorPaletteType) GetColorByHS(hsv HSV) (Color, bool) {
	if colorID, exists := colorHSMap[HsTuple{hsv.H, hsv.S}]; exists {
		if color, exists := cp[colorID]; exists {
			return color, true
		}
	}
	return Color{}, false
}

func (cp ColorPaletteType) GetColorByID(id ColorID) (Color, bool) {
	if color, exists := cp[id]; exists {
		return color, true
	}
	return Color{}, false
}

func (cp ColorPaletteType) GetColorByRGB(rgb RGB) (Color, bool) {
	if colorID, exists := colorRGBMap[rgb]; exists {
		if color, exists := cp[colorID]; exists {
			return color, true
		}
	}
	return Color{}, false
}

func (cp ColorPaletteType) FilterType(t ColorType) ColorPaletteType {
	newCP := ColorPaletteType{}
	for _, c := range cp {
		if c.Type == t {
			newCP[c.ID] = c
		}
	}
	return newCP
}

func (cp ColorPaletteType) GetNext(c Color) Color {
	if c.Type == WhiteColor {
		cpSlice := cp.FilterType(WhiteColor).ToSortedSlice()
		return getNeighbor(cpSlice, c, func(c, i Color) bool {
			return i.Temperature < c.Temperature
		})
	} else {
		cpSlice := cp.FilterType(Multicolor).ToSortedSlice()
		return getNeighbor(cpSlice, c, func(c, i Color) bool {
			return i.ValueHSV.H < c.ValueHSV.H
		})
	}
}

func (cp ColorPaletteType) GetPrevious(c Color) Color {
	if c.Type == WhiteColor {
		cpSlice := reverse(cp.FilterType(WhiteColor).ToSortedSlice())
		return getNeighbor(cpSlice, c, func(c, i Color) bool {
			return i.Temperature > c.Temperature
		})
	} else {
		cpSlice := reverse(cp.FilterType(Multicolor).ToSortedSlice())
		return getNeighbor(cpSlice, c, func(c, i Color) bool {
			return i.ValueHSV.H > c.ValueHSV.H
		})
	}
}

func (cscs ColorSettingCapabilityState) ToColor() (Color, bool) {
	switch cscs.Instance {
	case TemperatureKCapabilityInstance:
		return ColorPalette.GetNearestColorByColorTemperature(cscs.Value.(TemperatureK)), true
	case HsvColorCapabilityInstance:
		hsv := cscs.Value.(HSV)
		color, exists := ColorPalette.GetColorByHS(hsv)
		if !exists {
			color = Color{
				Type:     Multicolor,
				ValueHSV: hsv,
				ValueRGB: hsvToRgb(hsv),
			}
		}
		return color, true

	case RgbColorCapabilityInstance:
		rgb := cscs.Value.(RGB)
		color, exists := ColorPalette.GetColorByRGB(rgb)
		if !exists {
			color = Color{
				Type:     Multicolor,
				ValueHSV: rgbToHsv(rgb),
				ValueRGB: rgb,
			}
		}
		return color, true
	}

	return Color{}, false
}

func rgbToHsv(rgb RGB) HSV {
	r, g, b := intToRgb(rgb)

	rf := float64(r) / 255 //RGB from 0 to 255
	gf := float64(g) / 255
	bf := float64(b) / 255

	min := math.Min(rf, math.Min(gf, bf)) //Min. value of RGB
	max := math.Max(rf, math.Max(gf, bf)) //Max. value of RGB
	del := max - min                      //Delta RGB value

	v := max

	var h, s float64

	//This is a gray, no chroma...
	if del == 0 {
		h = 0
		s = 0

		//Chromatic data...
	} else {
		s = del / max

		delR := (((max - rf) / 6) + (del / 2)) / del
		delG := (((max - gf) / 6) + (del / 2)) / del
		delB := (((max - bf) / 6) + (del / 2)) / del

		if rf == max {
			h = delB - delG
		} else if gf == max {
			h = (1.0 / 3.0) + delR - delB
		} else if bf == max {
			h = (2.0 / 3.0) + delG - delR
		}

		if h < 0 {
			h += 1
		}
		if h > 1 {
			h -= 1
		}
	}

	hInt := int(math.Round(h * 360))
	sInt := int(math.Round(s * 100))
	vInt := int(math.Round(v * 100))

	return HSV{hInt, sInt, vInt}
}

func hsvToRgb(hsv HSV) RGB {
	var rf, gf, bf float64

	hf := float64(hsv.H) / 360
	sf := float64(hsv.S) / 100
	vf := float64(hsv.V) / 100

	//HSV from 0 to 1
	if hsv.S == 0 {
		rf = vf
		gf = vf
		bf = vf

	} else {
		h := hf * 6
		if h == 6 {
			h = 0
		}

		//H must be < 1
		i := math.Floor(h)
		v1 := vf * (1 - sf)
		v2 := vf * (1 - sf*(h-i))
		v3 := vf * (1 - sf*(1-(h-i)))

		if i == 0 {
			rf = vf
			gf = v3
			bf = v1
		} else if i == 1 {
			rf = v2
			gf = vf
			bf = v1
		} else if i == 2 {
			rf = v1
			gf = vf
			bf = v3
		} else if i == 3 {
			rf = v1
			gf = v2
			bf = vf
		} else if i == 4 {
			rf = v3
			gf = v1
			bf = vf
		} else {
			rf = vf
			gf = v1
			bf = v2
		}
	}

	rgb := rgbToInt(int(rf*255), int(gf*255), int(bf*255))
	return rgb
}

func rgbToInt(r, g, b int) RGB {
	rgb := r
	rgb = (rgb << 8) + g
	rgb = (rgb << 8) + b
	return RGB(rgb)
}

func intToRgb(rgb RGB) (r, g, b int) {
	r = (int(rgb) >> 16) & 0xFF
	g = (int(rgb) >> 8) & 0xFF
	b = int(rgb) & 0xFF
	return r, g, b
}

// from http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
func temperatureToRgb(temperatureK TemperatureK) (r, g, b int) {
	if temperatureK < 600 {
		return 255, 0, 0 // resolve to red anything that is lower than 600K
	}
	var fr, fg, fb float64
	if ft := float64(temperatureK) / 100; ft <= 66 {
		fr = 255
		fg = 99.4708025861*math.Log(ft) - 161.1195681661
		if ft <= 19 {
			fb = 0
		} else {
			fb = 138.5177312231*math.Log(ft-10) - 305.0447927307
		}
	} else {
		fr = 329.698727446 * math.Pow(ft-60, -0.1332047592)
		fg = 288.1221695283 * math.Pow(ft-60, -0.0755148492)
		fb = 255
	}
	// keep float r, g, b in [0, 255]
	r = int(math.Min(math.Round(math.Max(fr, 0)), 255))
	g = int(math.Min(math.Round(math.Max(fg, 0)), 255))
	b = int(math.Min(math.Round(math.Max(fb, 0)), 255))
	return r, g, b

}

func reverse(c []Color) []Color {
	for i, j := 0, len(c)-1; i < j; i, j = i+1, j-1 {
		c[i], c[j] = c[j], c[i]
	}
	return c
}

func getNeighbor(c []Color, cl Color, f func(i, cl Color) bool) Color {
	if len(c) == 0 {
		return cl
	}

	if len(c) == 1 {
		return c[0]
	}

	for _, v := range c {
		if f(v, cl) {
			return v
		}
	}

	return cl
}

type ColorScene struct {
	ID   ColorSceneID `json:"id"`
	Name string       `json:"name,omitempty"`
}

func (cs ColorScene) ToColorSettingCapabilityState() ColorSettingCapabilityState {
	return cs.ID.ToColorSettingCapabilityState()
}

func (cs *ColorScene) fromProto(p *protos.ColorScene) {
	cs.ID = ColorSceneID(p.GetID())
	cs.Name = p.GetName()
}

func (cs *ColorScene) toProto() *protos.ColorScene {
	if knownScene, ok := KnownColorScenes[cs.ID]; ok {
		return &protos.ColorScene{ID: string(cs.ID), Name: knownScene.Name}
	}
	return nil
}

func (cs *ColorScene) toUserInfoProto() *common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene {
	if knownScene, ok := KnownColorScenes[cs.ID]; ok {
		return &common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene{ID: string(cs.ID), Name: knownScene.Name}
	}
	return nil
}

func (cs *ColorScene) fromUserInfoProto(p *common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene) {
	cs.ID = ColorSceneID(p.GetID())
	cs.Name = p.GetName()
}

type ColorSceneID string

func (id ColorSceneID) String() string {
	return string(id)
}

func (id ColorSceneID) isColorSettingValue() {}

func (id ColorSceneID) Clone() ColorSettingValue {
	return id
}

func (id ColorSceneID) ToColorSettingCapabilityState() ColorSettingCapabilityState {
	return ColorSettingCapabilityState{
		Instance: SceneCapabilityInstance,
		Value:    id,
	}
}

type ColorScenes []ColorScene

func (cs ColorScenes) AsMap() map[ColorSceneID]ColorScene {
	result := make(map[ColorSceneID]ColorScene)
	for _, scene := range cs {
		result[scene.ID] = scene
	}
	return result
}
