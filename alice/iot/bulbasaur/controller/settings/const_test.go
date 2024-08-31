package settings

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestKnownMementoKeys(t *testing.T) {
	configs := makeConfigs()
	for _, config := range configs {
		found := false
		for _, key := range KnownMementoKeys {
			if key == config.Key() {
				found = true
				break
			}
		}
		assert.True(t, found)
	}

}
