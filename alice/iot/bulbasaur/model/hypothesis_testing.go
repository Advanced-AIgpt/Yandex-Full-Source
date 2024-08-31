package model

func NewHypothesis() *Hypothesis {
	return &Hypothesis{}
}

func (h *Hypothesis) WithID(id uint32) *Hypothesis {
	h.ID = int32(id)
	return h
}

func (h *Hypothesis) WithType(t HypothesisType) *Hypothesis {
	h.Type = t
	return h
}

func (h *Hypothesis) WithDevice(d string) *Hypothesis {
	h.Devices = append(h.Devices, d)
	return h
}

func (h *Hypothesis) WithDevices(devices ...string) *Hypothesis {
	h.Devices = append(h.Devices, devices...)
	return h
}

func (h *Hypothesis) WithRoom(r string) *Hypothesis {
	h.Rooms = append(h.Rooms, r)
	return h
}

func (h *Hypothesis) WithGroup(g string) *Hypothesis {
	h.Groups = append(h.Groups, g)
	return h
}

func (h *Hypothesis) WithHypothesisValue(a HypothesisValue) *Hypothesis {
	h.Value = a
	return h
}

func NewActionHypothesis() *HypothesisValue {
	return &HypothesisValue{}
}

func (a *HypothesisValue) WithTarget(t HypothesisTarget) *HypothesisValue {
	a.Target = t
	return a
}

func (a *HypothesisValue) WithCapabilityType(t CapabilityType) *HypothesisValue {
	a.Type = t.String()
	a.Target = CapabilityTarget
	return a
}

func (a *HypothesisValue) WithPropertyType(t PropertyType) *HypothesisValue {
	a.Type = t.String()
	a.Target = PropertyTarget
	return a
}

func (a *HypothesisValue) WithInstance(i string) *HypothesisValue {
	a.Instance = i
	return a
}

func (a *HypothesisValue) WithValue(v interface{}) *HypothesisValue {
	a.Value = v
	return a
}

func (a *HypothesisValue) WithRelative(v RelativityType) *HypothesisValue {
	a.Relative = &v
	return a
}

func (a *HypothesisValue) WithUnit(u Unit) *HypothesisValue {
	a.Unit = &u
	return a
}
