package adapter

import (
	"sort"
)

func SortDeviceActionResultViews(views []DeviceActionResultView) []DeviceActionResultView {
	for _, view := range views {
		sort.Sort(CapabilityActionResultViewByTypeAndInstance(view.Capabilities))
	}
	sort.Sort(DeviceActionResultViewsByID(views))
	return views
}

type DeviceActionResultViewsByID []DeviceActionResultView

func (d DeviceActionResultViewsByID) Len() int {
	return len(d)
}

func (d DeviceActionResultViewsByID) Less(i, j int) bool {
	return d[i].ID < d[j].ID
}

func (d DeviceActionResultViewsByID) Swap(i, j int) {
	d[i], d[j] = d[j], d[i]
}

type CapabilityActionResultViewByTypeAndInstance []CapabilityActionResultView

func (c CapabilityActionResultViewByTypeAndInstance) Len() int {
	return len(c)
}

func (c CapabilityActionResultViewByTypeAndInstance) Less(i, j int) bool {
	if c[i].Type == c[j].Type {
		return c[i].State.Instance < c[j].State.Instance
	}
	return c[i].Type < c[j].Type
}

func (c CapabilityActionResultViewByTypeAndInstance) Swap(i, j int) {
	c[i], c[j] = c[j], c[i]
}

func SortDeviceInfoViews(views []DeviceInfoView) []DeviceInfoView {
	for _, view := range views {
		sort.Sort(CapabilityInfoViewsByTypeAndInstance(view.Capabilities))
		sort.Sort(PropertyInfoViewsByInstance(view.Properties))
	}
	sort.Sort(DeviceInfoViewsByID(views))
	return views
}

type DeviceInfoViewsByID []DeviceInfoView

func (d DeviceInfoViewsByID) Len() int {
	return len(d)
}

func (d DeviceInfoViewsByID) Less(i, j int) bool {
	return d[i].ID < d[j].ID
}

func (d DeviceInfoViewsByID) Swap(i, j int) {
	d[i], d[j] = d[j], d[i]
}

type CapabilityInfoViewsByTypeAndInstance []CapabilityInfoView

func (c CapabilityInfoViewsByTypeAndInstance) Len() int {
	return len(c)
}

func (c CapabilityInfoViewsByTypeAndInstance) Less(i, j int) bool {
	if c[i].Type == c[j].Type {
		return c[i].GetInstance() < c[j].GetInstance()
	}
	return c[i].Type < c[j].Type
}

func (c CapabilityInfoViewsByTypeAndInstance) Swap(i, j int) {
	c[i], c[j] = c[j], c[i]
}

type PropertyInfoViewsByInstance []PropertyInfoView

func (p PropertyInfoViewsByInstance) Len() int {
	return len(p)
}

func (p PropertyInfoViewsByInstance) Less(i, j int) bool {
	return p[i].Parameters.GetInstance() < p[j].Parameters.GetInstance()
}

func (p PropertyInfoViewsByInstance) Swap(i, j int) {
	p[i], p[j] = p[j], p[i]
}

func SortDeviceStateViews(views []DeviceStateView) []DeviceStateView {
	for _, view := range views {
		sort.Sort(CapabilityStateViewsByTypeAndInstance(view.Capabilities))
		sort.Sort(PropertyStateViewsByInstance(view.Properties))
	}
	sort.Sort(DeviceStateViewsByID(views))
	return views
}

type DeviceStateViewsByID []DeviceStateView

func (d DeviceStateViewsByID) Len() int {
	return len(d)
}

func (d DeviceStateViewsByID) Less(i, j int) bool {
	return d[i].ID < d[j].ID
}

func (d DeviceStateViewsByID) Swap(i, j int) {
	d[i], d[j] = d[j], d[i]
}

type CapabilityStateViewsByTypeAndInstance []CapabilityStateView

func (c CapabilityStateViewsByTypeAndInstance) Len() int {
	return len(c)
}

func (c CapabilityStateViewsByTypeAndInstance) Less(i, j int) bool {
	if c[i].Type == c[j].Type {
		return c[i].State.GetInstance() < c[j].State.GetInstance()
	}
	return c[i].Type < c[j].Type
}

func (c CapabilityStateViewsByTypeAndInstance) Swap(i, j int) {
	c[i], c[j] = c[j], c[i]
}

type PropertyStateViewsByInstance []PropertyStateView

func (p PropertyStateViewsByInstance) Len() int {
	return len(p)
}

func (p PropertyStateViewsByInstance) Less(i, j int) bool {
	return p[i].State.GetInstance() < p[j].State.GetInstance()
}

func (p PropertyStateViewsByInstance) Swap(i, j int) {
	p[i], p[j] = p[j], p[i]
}
