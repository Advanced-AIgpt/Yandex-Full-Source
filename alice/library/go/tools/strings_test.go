package tools

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestStandardizeSpaces(t *testing.T) {
	assert.Equal(t, "this is simple string", StandardizeSpaces("this is simple string"))
	assert.Equal(t, "this is simple string", StandardizeSpaces("\n\tthis is simple string  \n"))
	assert.Equal(t, "this is simple string", StandardizeSpaces("\n\tthis    is   simple \n\n\n \t string  \n"))
}

func TestIsAlphanumericEqual(t *testing.T) {
	assert.True(t, IsAlphanumericEqual("кОмНаТа", "\n\n\t@@комната****"))
	assert.True(t, IsAlphanumericEqual("11222ДоМо вой228", "             **(((11222домо            вой228)))                 "))
}

func TestIntersect(t *testing.T) {
	s1 := []string{"A", "B", "C"}
	s2 := []string{"B", "C"}
	s3 := []string{"A"}
	assert.Equal(t, s2, Intersect(s1, s2))
	assert.Equal(t, s3, Intersect(s1, s3))
	assert.Equal(t, []string{}, Intersect(s2, s3))
}
