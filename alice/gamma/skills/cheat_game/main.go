package main

import (
	"math/rand"
	"time"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

func main() {
	source := rand.NewSource(time.Now().Unix())
	random := rand.New(source)
	sdk.StartSkill(NewCheatSkill(*random), sdk.DefaultOptions{})
}
