package main

import (
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/random"
)

const (
	// userID ids
	yndxQuasarTest2 uint64 = 686859036 // login yndx.quasar.test.2
	yndxSmartHome1  uint64 = 870642880 // login yndx-smart-home1
	avSalnikov      uint64 = 50515986  // login a-v-salnikov
	timaShunin      uint64 = 78442638  // login tima.shunin
	norchine08      uint64 = 88774996  // login norchine08
)

var speakerName = map[model.DeviceType]string{
	model.YandexStationDeviceType:      "Яндекс Станция",
	model.YandexStation2DeviceType:     "Яндекс Станция 2",
	model.YandexModuleDeviceType:       "Яндекс Модуль",
	model.DexpSmartBoxDeviceType:       "Колонка Dexp",
	model.IrbisADeviceType:             "Колонка Irbis",
	model.LGXBoomDeviceType:            "Колонка LG",
	model.ElariSmartBeatDeviceType:     "Колонка Elari",
	model.YandexStationMiniDeviceType:  "Яндекс Мини",
	model.JetSmartMusicDeviceType:      "Колонка Jet Smart Music",
	model.PrestigioSmartMateDeviceType: "Колонка Prestigio Smartmate",
	model.DigmaDiHomeDeviceType:        "Колонка Digma Di Home",
	model.JBLLinkPortableDeviceType:    "JBL Link Portable",
	model.JBLLinkMusicDeviceType:       "JBL Link Music",
}

func memeNameGenerator() string {
	adjectives := []string{
		"Волшебный",
		"Мемный",
		"Невероятный",
		"Удобный",
		"Радиоактивный",
		"Неадекватный",
		"Разрушительный",
		"Низкокалорийный",
		"Натуральный",
		"Глубинный",
		"Беспричинный",
		"Пёсий",
		"Клавовый",
		"Туёвый",
		"Любимый",
		"Сочный",
		"Знойный",
		"Голубиный",
		"Бобриный",
		"Аюковый",
		"Алисий",
		"Шелковистый",
		"Подкустовый",
		"Реактивный",
		"Данечкин",
		"Ковидный",
		"Молочный",
		"Обжаренный",
		"Маслянистый",
		"Хвойный",
		"Рифленый",
	}
	nouns := []string{
		"Обнулятор",
		"Поливатор",
		"Проверятор",
		"Устранитель",
		"Стабилизатор",
		"Измеритель",
		"Увольнятор",
		"Усиливатель",
		"Сокрушитель",
		"Стерилизатор",
		"Орошатор",
		"Зондатор",
		"Ласкатор",
		"Плантатор",
		"Имитатор",
		"Симулятор",
		"Увлажнятор",
		"Омаратор",
		"Нерестатор",
		"Омыватор",
		"Гемморатор",
		"Конфискатор",
		"Излучатель",
		"Покоритель",
		"Глистомёт",
		"Концентрат",
		"Выползень",
		"Гвоздь",
		"Тунец",
		"Голубец",
		"Клавдезь",
		"Димон",
		"Катышек",
	}
	subnouns := []string{
		"Тимофея",
		"Аюки",
		"цветов",
		"устройств",
		"дома",
		"яичницы",
		"бутеров",
		"ванной",
		"команды",
		"коллег",
		"грусти",
		"радости",
		"лулзов",
		"ехидны",
		"бобровой струи",
		"смысла жизни",
		"моих слез",
		"твоего отпуска",
		"колоска",
		"капибар",
		"чсв",
		"моей мечты",
		"из ада",
		"бегемота",
		"из болота",
		"за углом",
		"котят",
		"пупка",
	}
	if random.FlipCoin() {
		return fmt.Sprintf("%s %s", random.Choice(adjectives), random.Choice(nouns))
	} else {
		return fmt.Sprintf("%s %s", random.Choice(nouns), random.Choice(subnouns))
	}
}
