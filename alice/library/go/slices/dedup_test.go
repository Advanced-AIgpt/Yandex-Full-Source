package slices

import (
	"github.com/stretchr/testify/assert"
	"testing"
)

func TestDedupUInt64(t *testing.T) {
	testCases := []struct {
		name     string
		input    []uint64
		expected []uint64
	}{
		{
			name:     "empty",
			input:    []uint64{},
			expected: []uint64{},
		},
		{
			name:     "one el",
			input:    []uint64{2},
			expected: []uint64{2},
		},
		{
			name:     "several distinct element",
			input:    []uint64{3, 24, 1},
			expected: []uint64{3, 24, 1},
		},
		{
			name:     "remove duplicates",
			input:    []uint64{3, 24, 3, 1, 4},
			expected: []uint64{3, 24, 1, 4},
		},
		{
			name:     "remove duplicates",
			input:    []uint64{3, 24, 3, 1, 4},
			expected: []uint64{3, 24, 1, 4},
		},
		{
			name:     "remove duplicates 2",
			input:    []uint64{3, 24, 3, 1, 4, 1, 24, 9},
			expected: []uint64{3, 24, 1, 4, 9},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			assert.Equal(t, tc.expected, DedupUint64sKeepOrder(tc.input))
		})
	}
}
