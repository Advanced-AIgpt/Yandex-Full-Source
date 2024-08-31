package random

import (
	"math/rand"

	"a.yandex-team.ru/alice/library/go/tools"
)

func FlipCoin() bool {
	return rand.Intn(2) > 0
}

func FlipCube(n int) int {
	return rand.Intn(n)
}

func HashChoice(slice []string, seed string) string {
	return slice[int(tools.HuidifyString(seed))%len(slice)]
}

func Choice(slice []string) string {
	return slice[FlipCube(len(slice))]
}

func Choices(n int, slice []string) (result []string) {
	if n < 1 {
		return result
	}
	if n > len(slice) {
		panic("too many choices")
	}
	seen := make(map[string]bool)
	for len(result) < n {
		c := Choice(slice)
		if _, ok := seen[c]; ok {
			continue
		}
		seen[c] = true
		result = append(result, c)
	}
	return result
}

func RandRange(min, max int) int {
	if max <= min {
		return 0
	}
	return rand.Intn(max-min) + min
}
