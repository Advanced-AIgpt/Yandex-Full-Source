package callback

type DiscoveryFilterType string

const (
	DiscoveryDeviceTypeFilterType DiscoveryFilterType = "device_type"
	DiscoveryDeviceIDFilterType   DiscoveryFilterType = "device_id"
	DiscoveryDefaultFilterType    DiscoveryFilterType = "default"
)
