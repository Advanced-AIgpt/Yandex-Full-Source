package data

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/library/go/random"

	"strings"
)

func GenerateRoom() (room model.Room) {
	for strings.TrimSpace(room.Name) == "" {
		room.Name = testing.RandOnlyCyrillicString(random.RandRange(10, 20))
	}
	return room
}
