package sorting

import (
	"sort"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestStringsSorting(t *testing.T) {
	items := []string{"A", "Z", "B", "a1"}
	expected := []string{"A", "a1", "B", "Z"}
	sort.Sort(CaseInsensitiveStringsSorting(items))
	assert.Equal(t, expected, items)
}
