package inflector

type IInflector interface {
	Inflect(text string, grams []string) (Inflection, error)
	InflectWithHints(tokens []string, hints []string, grams []string) (Inflection, error)
}

func TryInflect(inf IInflector, text string, grams []string) Inflection {
	inflection, err := inf.Inflect(text, grams)
	if err != nil {
		inflection = Inflection{
			Im:   text,
			Rod:  text,
			Dat:  text,
			Vin:  text,
			Tvor: text,
			Pr:   text,
		}
	}
	return inflection
}
