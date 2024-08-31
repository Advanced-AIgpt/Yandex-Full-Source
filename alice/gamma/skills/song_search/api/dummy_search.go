package api

import (
	"strings"
)

type DummySearch struct{}

func (api *DummySearch) Search(line string) ([]SearchResult, error) {
	switch strings.ToLower(line) {
	case "цой спокойная ночь":
		return []SearchResult{
			{
				Artist: "Кино",
				Title:  "Ночь",
				Text:   "И эта ночь и ее электрический голос манит меня к себе\nИ я не знаю, как мне прожить следующий день.",
				URL:    "https://music.yandex.ru/album/10010/track/105094",
			},
			{
				Artist: "Любэ",
				Title:  "Ночь",
				Text:   "Был душой я молод, а теперь старик\nБыл душой я молод, а теперь старик",
				URL:    "https://music.yandex.ru/album/5060850/track/2215049",
			},
			{
				Artist:   "Кровосток",
				Title:    "Ночь",
				Text:     "Триггер на текст. Хороший текст. Внизу по ссылке.",
				URL:      "https://music.yandex.ru/album/6554940/track/30767173",
				Explicit: true,
			},
			{
				Artist: "Мат",
				Title:  "Мат",
				Text:   "Тригер на превью",
				URL:    "https://music.yandex.ru/album/-1/track/-1",
			},
		}, nil
	case "я что-то нажала и все исчезло":
		return []SearchResult{
			{
				Artist: "Комсомольск",
				Title:  "Всё исчезло",
				Text:   "Смотри Илон Маск я тут что-то нажала\nИ всё исчезло\nИ всё исчезло",
				URL:    "https://music.yandex.ru/album/5468632/track/41638346",
			},
		}, nil
	default:
		return nil, nil
	}
}
