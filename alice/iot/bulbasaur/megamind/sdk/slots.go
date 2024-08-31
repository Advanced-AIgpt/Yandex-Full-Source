package sdk

// GranetSlot allows reusing slot parsing logic across different processors.
// GranetSlots are intended to be used in GranetFrame's implementations.
// Use frames.ScenarioLaunchFrame as inspiration for your frames.
type GranetSlot interface {
	// Name returns the name of the slot.
	Name() string

	// Type returns the actual type of the slot. It must be one of the types returned by SupportedTypes method.
	Type() string

	// SupportedTypes returns a list of types supported by this slot.
	// Granet allows storing values of different types in slots with the same name.
	SupportedTypes() []string

	// New must create an empty instance of the slot
	New(slotType string) GranetSlot

	// SetValue must parse the given string value and save it to the slot.
	SetValue(value string) error

	// SetType must set actual type of the slot checking if the type is one of the SupportedTypes.
	SetType(slotType string) error
}
