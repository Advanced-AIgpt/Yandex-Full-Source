package testing

import (
	"math/rand"
	"unicode"

	"a.yandex-team.ru/alice/library/go/random"
)

var LatinLetterBytes = []rune("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890 ")
var CyrillicLetterBytes = []rune("абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ1234567890 ")
var OnlyCyrillicLetterBytes = []rune("абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ ")
var VersionLetterBytes = []rune("1234567890._")

func RandString(n int) string {
	if random.FlipCoin() {
		return RandCyrillicWithNumbersString(n)
	} else {
		return RandLatinString(n)
	}
}

func RandCyrillicWithNumbersString(n int) string {
	return RandAlphabetString(n, CyrillicLetterBytes)
}

func RandOnlyCyrillicString(n int) string {
	return RandAlphabetString(n, OnlyCyrillicLetterBytes)
}

func RandLatinString(n int) string {
	return RandAlphabetString(n, LatinLetterBytes)
}

func RandAlphabetString(n int, letters []rune) string {
	b := make([]rune, n)
	for i := range b {
		b[i] = letters[rand.Intn(len(letters))]
	}
	if n > 0 {
		for unicode.IsSpace(b[0]) {
			b[0] = letters[rand.Intn(len(letters))]
		}
		for unicode.IsSpace(b[n-1]) {
			b[n-1] = letters[rand.Intn(len(letters))]
		}
	}

	return string(b)
}
