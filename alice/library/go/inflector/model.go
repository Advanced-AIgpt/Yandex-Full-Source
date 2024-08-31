package inflector

import "strings"

type Inflection struct {
	Im   string
	Rod  string
	Dat  string
	Vin  string
	Tvor string
	Pr   string
}

func (i Inflection) ToLower() Inflection {
	return Inflection{
		strings.ToLower(i.Im),
		strings.ToLower(i.Rod),
		strings.ToLower(i.Dat),
		strings.ToLower(i.Vin),
		strings.ToLower(i.Tvor),
		strings.ToLower(i.Pr),
	}
}
