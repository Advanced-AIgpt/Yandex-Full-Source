package miotspec

type InstanceSpecResult struct {
	Type        string
	Description string
	Services    []Service
}

type Property struct {
	Iid         int
	Type        string
	Description string
	Format      string
	Access      []string
	Unit        string
	ValueRange  []float64 `json:"value-range"`
	ValueList   []Value   `json:"value-list"`
}

type Action struct {
	Iid         int
	Type        string
	Description string
	In          []interface{}
	Out         []interface{}
}

type Event struct {
	Iid         int
	Type        string
	Description string
}

type Value struct {
	Value       int
	Description string
}

type Service struct {
	Iid         int
	Type        string
	Description string
	Properties  []Property
	Actions     []Action
	Events      []Event
}
