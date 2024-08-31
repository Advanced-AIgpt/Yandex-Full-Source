package libmegamind

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

// Probably the string will look like this:
// "[{\"user.iot.device\":\"66666666-1337-abcd-8008-123456654321\"},{\"iot\":\"66666666-1337-abcd-8008-123456231372\"}]"
// Not sure about escape chars
func TestVariantsValues(t *testing.T) {
	variants := SlotVariants(`[{"user.iot.device":"66666666-1337-abcd-8008-123456654321"},{"user.iot.device":"66666666-1337-abcd-8008-123456231372"}]`)
	expectedVariants := []string{
		"66666666-1337-abcd-8008-123456654321",
		"66666666-1337-abcd-8008-123456231372",
	}
	actualVariants, err := variants.Values()
	assert.NoError(t, err)
	assert.ElementsMatch(t, expectedVariants, actualVariants)
}
