package model

type TestCapability struct {
	ICapability // only use for suggestions
}

func NewCapability(cType CapabilityType) *TestCapability {
	return &TestCapability{ICapability: MakeCapabilityByType(cType)}
}

func (c *TestCapability) WithRetrievable(retrievable bool) *TestCapability {
	c.SetRetrievable(retrievable)
	return c
}

func (c *TestCapability) WithState(state ICapabilityState) *TestCapability {
	c.SetState(state)
	return c
}

func (c *TestCapability) WithParameters(parameters ICapabilityParameters) *TestCapability {
	c.SetParameters(parameters)
	return c
}
