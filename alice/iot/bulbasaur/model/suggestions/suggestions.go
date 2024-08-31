package suggestions

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

var RoomNames []string

var GroupNames map[model.DeviceType][]string
var DefaultGroupNames []string
var MultiroomGroupNames []string

var DeviceNames map[model.DeviceType][]string
var DefaultDeviceNames []string

var HouseholdNames []string

func DefaultDeviceNameByType(deviceType model.DeviceType) string {
	if suggestions, ok := DeviceNames[deviceType]; ok {
		return suggestions[0]
	}
	return DefaultDeviceNames[0]
}

func GroupNameSuggests(groupType model.DeviceType) []string {
	if model.MultiroomSpeakers[groupType] || groupType == model.SmartSpeakerDeviceType {
		return MultiroomGroupNames
	}
	if specialTypeNames, ok := GroupNames[groupType]; ok {
		return specialTypeNames
	}
	return DefaultGroupNames
}

func init() {
	RoomNames = []string{"Балкон", "Ванная", "Ванная комната", "Гардеробная", "Гостиная", "Детская", "Зал", "Кабинет", "Коридор", "Кухня", "Прихожая", "Спальня", "Столовая", "Туалет"}
	HouseholdNames = []string{"Дом", "Квартира", "Дача"}

	DefaultGroupNames = []string{"Группа устройств", "Бытовые приборы", "Люстра", "Освещение", "Розетки", "Верхний свет"}
	MultiroomGroupNames = []string{"Группа колонок", "Мультирум"}

	GroupNames = make(map[model.DeviceType][]string)
	GroupNames[model.LightDeviceType] = []string{"Люстра", "Новогоднее освещение", "Ночное освещение", "Подсветка"}
	GroupNames[model.LightCeilingDeviceType] = []string{"Люстры", "Светильники", "Споты", "Ночники"}
	GroupNames[model.LightLampDeviceType] = []string{"Лампы", "Светильники", "Настольные лампы", "Ночники"}
	GroupNames[model.LightStripDeviceType] = []string{"Ленты", "Диодные ленты", "Подсветки", "Фартуки"}
	GroupNames[model.SocketDeviceType] = []string{"Бытовые приборы", "Розетки", "Электричество"}
	GroupNames[model.MediaDeviceDeviceType] = []string{"Акустика", "Видео", "Кинотеатр"}
	GroupNames[model.ThermostatDeviceType] = []string{"Климат", "Отопление", "Вентиляция"}
	GroupNames[model.AcDeviceType] = []string{"Климат", "Вентиляция", "Кондиционеры"}
	GroupNames[model.TvDeviceDeviceType] = []string{"Телевизоры", "Медиа", "Кинотеатр"}
	GroupNames[model.ReceiverDeviceType] = []string{"Ресиверы"}
	GroupNames[model.TvBoxDeviceType] = []string{"Приставки"}
	GroupNames[model.SwitchDeviceType] = []string{"Ночное освещение", "Подсветка", "Выключатели"}
	GroupNames[model.CurtainDeviceType] = []string{"Шторы", "Кулисы", "Жалюзи"}
	GroupNames[model.PurifierDeviceType] = []string{"Очистители", "Климат", "Вентиляция"}
	GroupNames[model.HumidifierDeviceType] = []string{"Увлажнители", "Климат", "Вентиляция"}
	GroupNames[model.VacuumCleanerDeviceType] = []string{"Пылесосы"}
	GroupNames[model.CoffeeMakerDeviceType] = []string{"Кофеварки", "Кофемашины"}
	GroupNames[model.WashingMachineDeviceType] = []string{"Стиральные машины", "Стиралки"}
	GroupNames[model.DishwasherDeviceType] = []string{"Посудомоечные машины", "Посудомойки"}
	GroupNames[model.MulticookerDeviceType] = []string{"Мультиварки", "Мультишефы"}
	GroupNames[model.RefrigeratorDeviceType] = []string{"Холодильники"}
	GroupNames[model.FanDeviceType] = []string{"Вентиляторы"}
	GroupNames[model.IronDeviceType] = []string{"Утюги"}
	GroupNames[model.PetFeederDeviceType] = []string{"Кормушки"}
	GroupNames[model.HubDeviceType] = []string{"Пульты", "Умные пульты"}
	GroupNames[model.CookingDeviceType] = []string{"Кухонные приборы"}
	GroupNames[model.KettleDeviceType] = []string{"Чайники", "Самовары"}
	GroupNames[model.OtherDeviceType] = []string{"Устройства"}
	GroupNames[model.OpenableDeviceType] = []string{"Двери"}
	GroupNames[model.RemoteCarDeviceType] = []string{"Автомобили", "Машины"}
	GroupNames[model.SensorDeviceType] = []string{"Датчики температуры", "Датчики давления", "Датчики влажности", "Датчики освещенности", "Датчики движения", "Датчики двери", "Датчики воздуха"}
	GroupNames[model.CameraDeviceType] = []string{"Камеры"}

	DefaultDeviceNames = []string{"Моё устройство"}
	DeviceNames = make(map[model.DeviceType][]string)

	DeviceNames[model.LightDeviceType] = []string{"Лампа", "Лампочка", "Люстра", "Настольная лампа", "Ночник", "Светильник", "Умная лампочка"}
	DeviceNames[model.LightCeilingDeviceType] = []string{"Люстра", "Светильник", "Спот", "Ночник"}
	DeviceNames[model.LightLampDeviceType] = []string{"Лампа", "Светильник", "Настольная лампа", "Ночник"}
	DeviceNames[model.LightStripDeviceType] = []string{"Лента", "Диодная лента", "Подсветка", "Фартук"}
	DeviceNames[model.SocketDeviceType] = []string{"Розетка", "Удлинитель", "Увлажнитель", "Телевизор", "Компьютер", "Вентилятор", "Гирлянда", "Вытяжка", "Зарядка", "Обогреватель"}
	DeviceNames[model.MediaDeviceDeviceType] = []string{"Плеер", "Приставка", "Саундбар", "Телевизор"}
	DeviceNames[model.SwitchDeviceType] = []string{"Выключатель", "Люстра", "Переключатель", "Светильник", "Тумблер"}
	DeviceNames[model.ThermostatDeviceType] = []string{"Водонагреватель", "Кондиционер", "Обогреватель", "Теплый пол", "Термостат", "Конвектор"}
	DeviceNames[model.OtherDeviceType] = []string{"Стиральная машина", "Пылесос", "Холодильник", "Вентилятор", "Климат", "Выключатель"}
	DeviceNames[model.AcDeviceType] = []string{"Кондиционер"}
	DeviceNames[model.TvDeviceDeviceType] = []string{"Телевизор", "Телек", "Ящик"}
	DeviceNames[model.ReceiverDeviceType] = []string{"Ресивер"}
	DeviceNames[model.TvBoxDeviceType] = []string{"Приставка"}
	DeviceNames[model.CookingDeviceType] = []string{"Гриль", "Мультиварка", "Пароварка", "Самовар", "Холодильник"}
	DeviceNames[model.KettleDeviceType] = []string{"Самовар", "Термопот", "Чайник"}
	DeviceNames[model.CoffeeMakerDeviceType] = []string{"Кофеварка", "Кофемашина"}
	DeviceNames[model.HubDeviceType] = []string{"Пульт", "Умный пульт"}
	DeviceNames[model.OpenableDeviceType] = []string{"Дверь", "Ворота", "Замок"}
	DeviceNames[model.CurtainDeviceType] = []string{"Шторы", "Жалюзи", "Тюль", "Занавески", "Гардины"}
	DeviceNames[model.VacuumCleanerDeviceType] = []string{"Пылесос", "Робот пылесос"}
	DeviceNames[model.PurifierDeviceType] = []string{"Очиститель", "Очиститель воздуха"}
	DeviceNames[model.HumidifierDeviceType] = []string{"Увлажнитель", "Увлажнитель воздуха"}
	DeviceNames[model.WashingMachineDeviceType] = []string{"Стиральная машина", "Стиралка"}
	DeviceNames[model.DishwasherDeviceType] = []string{"Посудомоечная машина", "Посудомойка"}
	DeviceNames[model.MulticookerDeviceType] = []string{"Мультиварка", "Мультишеф"}
	DeviceNames[model.RefrigeratorDeviceType] = []string{"Холодильник"}
	DeviceNames[model.FanDeviceType] = []string{"Вентилятор"}
	DeviceNames[model.IronDeviceType] = []string{"Утюг"}
	DeviceNames[model.PetFeederDeviceType] = []string{"Кормушка"}
	DeviceNames[model.RemoteCarDeviceType] = []string{"Автомобиль", "Машина"}
	DeviceNames[model.SensorDeviceType] = []string{"Датчик температуры", "Датчик давления", "Датчик влажности", "Датчик освещенности", "Датчик движения", "Датчик двери", "Датчик воздуха", "Датчик"}
	DeviceNames[model.CameraDeviceType] = []string{"Камера"}
}
