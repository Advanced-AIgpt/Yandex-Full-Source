package tuya

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestIRCategoriesToDeviceMap(t *testing.T) {
	for irCategory, deviceType := range IRCategoriesToDeviceTypeMap {
		irCategoryInReverseMap, found := DeviceTypeToIRCategoriesMap[deviceType]
		assert.Equal(t, true, found)
		assert.Equal(t, irCategory, irCategoryInReverseMap)
	}
}

func TestIsAllowedBrandNameRegex(t *testing.T) {
	assert.True(t, isAllowedBrandNameRegex.MatchString("Bang&Olufsen"))
	assert.True(t, isAllowedBrandNameRegex.MatchString("Tuya"))
	assert.True(t, isAllowedBrandNameRegex.MatchString("Samsung"))

	assert.False(t, isAllowedBrandNameRegex.MatchString("***superBrand***"))
	assert.False(t, isAllowedBrandNameRegex.MatchString("N@GIBATOR"))
	assert.False(t, isAllowedBrandNameRegex.MatchString("my (old) new brand"))
}
