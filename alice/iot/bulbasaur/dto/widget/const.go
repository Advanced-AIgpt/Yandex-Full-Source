package widget

const LightingItemViewTypeRoom = "room"
const LightingItemViewTypeGroup = "group"
const LightingItemViewTypeDevice = "device"

var KnownRoomNamesURL = map[RoomName]string{
	"Кухня":             "https://avatars.mds.yandex.net/get-iot/icons-rooms-kitchen.svg",
	"Гостинная":         "https://avatars.mds.yandex.net/get-iot/icons-rooms-living-room.svg",
	"Детская":           "https://avatars.mds.yandex.net/get-iot/icons-rooms-childrens-room.svg",
	"Ванная":            "https://avatars.mds.yandex.net/get-iot/icons-rooms-bathroom.svg",
	"Спальня":           "https://avatars.mds.yandex.net/get-iot/icons-rooms-bedroom.svg",
	"Дефолтная комната": "https://avatars.mds.yandex.net/get-iot/icons-rooms-default-room.svg",
}
