package inflector

import (
	"fmt"
	"strings"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Mock struct {
	logger  log.Logger
	answers map[string]Inflection
}

func NewInflectorMock(logger log.Logger) *Mock {
	m := Mock{
		logger:  logger,
		answers: make(map[string]Inflection),
	}
	m.logger.Infof("Mock inflector client created")
	return &m
}

func (m *Mock) WithInflection(text string, inflection Inflection) *Mock {
	m.answers[text] = inflection
	return m
}

func (m *Mock) Inflect(text string, casuses []string) (Inflection, error) {
	m.logger.Infof("Inflecting %s", text)
	if inflection, ok := m.answers[text]; !ok {
		return Inflection{}, xerrors.Errorf("inflector response not found for text %s", text)
	} else {
		return inflection, nil
	}
}

func (m *Mock) InflectWithHints(tokens []string, grams []string, casuses []string) (Inflection, error) {
	textWithGrams := make([]string, 0, len(grams))
	for i, word := range tokens {
		textWithGrams = append(textWithGrams, fmt.Sprintf("%s{grams=%s}", word, grams[i]))
	}
	return m.Inflect(strings.Join(textWithGrams[:], " "), casuses)
}
