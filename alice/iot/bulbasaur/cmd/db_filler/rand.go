package main

import (
	"fmt"
	"math/rand"
)

var cyrillicLetterBytes = []rune("абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ1234567890 ")

func rng(n int) (r []int) {
	for i := 0; i < n; i++ {
		r = append(r, i)
	}
	return r
}

func difference(slice1 []int, slice2 []int) []int {
	var diff []int

	// Loop two times, first to find slice1 strings not in slice2,
	// second loop to find slice2 strings not in slice1
	for i := 0; i < 2; i++ {
		for _, s1 := range slice1 {
			found := false
			for _, s2 := range slice2 {
				if s1 == s2 {
					found = true
					break
				}
			}
			// String not found. We add it to return slice
			if !found {
				diff = append(diff, s1)
			}
		}
		// Swap the slices, only if it was the first loop
		if i == 0 {
			slice1, slice2 = slice2, slice1
		}
	}

	return diff
}

func pickRandomFew(n int, l []int) (picked []int) {
	if n == len(l) {
		return l
	}
	if n > len(l) {
		panic(fmt.Sprintf("cannot pick more than %d", l))
	}
	seen := make(map[int]bool)
	for len(picked) < n {
		candidate := rand.Intn(len(l))
		if _, ok := seen[candidate]; ok {
			continue
		}
		seen[candidate] = true
		picked = append(picked, l[candidate])
	}
	return picked
}

func pickRandomFewAndRemove(n int, l []int) (picked, rest []int) {
	picked = pickRandomFew(n, l)
	rest = difference(picked, l)
	return
}

func randCyrillicString(n int) string {
	return randAlphabetString(n, cyrillicLetterBytes)
}

func randAlphabetString(n int, letters []rune) string {
	b := make([]rune, n)
	for i := range b {
		b[i] = letters[rand.Intn(len(letters))]
	}
	return string(b)
}

func randInt(n int) int {
	if n == 0 {
		return n
	}
	return rand.Intn(n)
}

func randRange(min, max int) int {
	if max <= min {
		return 0
	}
	return rand.Intn(max-min) + min
}

func choice(slice []string) string {
	return slice[rand.Intn(len(slice))]
}

func flipCoin() bool {
	return rand.Intn(2) > 0
}
