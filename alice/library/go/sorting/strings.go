package sorting

import (
	"strings"
)

type CaseInsensitiveStringsSorting []string

func (s CaseInsensitiveStringsSorting) Len() int { return len(s) }
func (s CaseInsensitiveStringsSorting) Less(i, j int) bool {
	return strings.ToLower(s[i]) < strings.ToLower(s[j])
}
func (s CaseInsensitiveStringsSorting) Swap(i, j int) { s[i], s[j] = s[j], s[i] }
