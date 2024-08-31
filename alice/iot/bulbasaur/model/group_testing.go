package model

func NewGroup(name string) *Group {
	return &Group{
		Name:    name,
		Aliases: []string{},
		Devices: []string{},
	}
}

func (g *Group) WithID(id string) *Group {
	g.ID = id
	return g
}

func (g *Group) WithAliases(aliases ...string) *Group {
	g.Aliases = append(g.Aliases, aliases...)
	return g
}

func (g *Group) WithType(dt DeviceType) *Group {
	g.Type = dt
	return g
}

func (g *Group) WithDevices(deviceIDs ...string) *Group {
	g.Devices = append(g.Devices, deviceIDs...)
	return g
}
