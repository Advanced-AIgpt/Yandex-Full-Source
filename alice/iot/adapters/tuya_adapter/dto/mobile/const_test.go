package mobile

import (
	"testing"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"github.com/stretchr/testify/assert"
)

func TestMapConsistency(t *testing.T) {
	for _, irCategory := range KnownIRCategories {
		// check if for every category in map here is tuya name in tuya package
		irCategoryNameReverseMap, found := tuya.KnownIrCategoryNames[tuya.IrCategoryID(irCategory.ID)]
		assert.Equal(t, true, found)
		assert.Equal(t, irCategory.Name, irCategoryNameReverseMap)
	}
}
