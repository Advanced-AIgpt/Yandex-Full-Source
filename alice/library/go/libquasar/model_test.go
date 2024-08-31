package libquasar

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestJoinSplitVersions(t *testing.T) {

	t.Run("split_bad_version", func(t *testing.T) {
		at := assert.New(t)
		var v DeviceVersions
		err := v.FromJoined("/123")
		at.Error(err)
	})

	input := []struct {
		name     string
		versions DeviceVersions
	}{
		{
			name:     "empty",
			versions: DeviceVersions{},
		},
		{
			name:     "one device",
			versions: DeviceVersions{{DeviceID: "asd", Version: "fff"}},
		},
		{
			name:     "two devices",
			versions: DeviceVersions{{DeviceID: "12ewd", Version: "134"}, {DeviceID: "f34ef", Version: "8gierulfg"}},
		},
	}

	for _, test := range input {
		t.Run(test.name, func(t *testing.T) {
			at := assert.New(t)

			version := test.versions.JoinVersions()

			var restoredVersions DeviceVersions
			err := restoredVersions.FromJoined(version)
			at.NoError(err)
			at.Equal(test.versions, restoredVersions)
		})
	}
}

func TestGroupInfoUnmarshal(t *testing.T) {
	rawGroupB := `{
        "config": {
          "music": {
            "play_to": "leader"
          }
        },
        "devices": [
          {
            "id": "04007884c9140c0909cf",
            "platform": "yandexstation",
            "role": "leader"
          },
          {
            "id": "D003400004BWZW",
            "platform": "yandexmodule_2",
            "role": "follower"
          }
        ],
        "id": 22346,
        "name": "04007884c9140c0909cf-D003400004BWZW",
        "secret": "04007884c9140c0909cf-D003400004BWZW"
      }`
	var groupInfo GroupInfo
	err := json.Unmarshal([]byte(rawGroupB), &groupInfo)
	assert.NoError(t, err)
	assert.Equal(t, uint64(22346), groupInfo.ID)
}
