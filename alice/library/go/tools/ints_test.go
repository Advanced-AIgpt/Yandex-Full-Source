package tools

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

var intSliceTest = []int{2, 5, 8, 9, 33, 4, 4, 4, 99}

func TestIntSliceMin(t *testing.T) {
	assert.Equal(t, 2, IntSliceMin(intSliceTest))
}

func TestIntSliceMax(t *testing.T) {
	assert.Equal(t, 99, IntSliceMax(intSliceTest))
}
