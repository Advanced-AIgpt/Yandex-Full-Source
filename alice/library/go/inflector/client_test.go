package inflector

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestInflectorClient_Inflect(t *testing.T) {
	i := Client{}

	type testCase struct {
		text     string
		expected Inflection
	}

	check := func(tc testCase) func(*testing.T) {
		return func(t *testing.T) {
			res, err := i.Inflect(tc.text, []string{Im, Rod, Dat, Vin, Tvor, Pr})
			assert.Equal(t, tc.expected, res)
			assert.NoError(t, err)
		}
	}

	testCases := []struct {
		name  string
		input testCase
	}{
		{
			name: "розовый слон 1",
			input: testCase{
				text: "розовый слон 1",
				expected: Inflection{
					Im:   "розовый слон 1",
					Rod:  "розового слона 1",
					Dat:  "розовому слону 1",
					Vin:  "розового слона 1",
					Tvor: "розовым слоном 1",
					Pr:   "розовом слоне 1",
				},
			},
		},
		{
			name: "очиститель воздуха",
			input: testCase{
				text: "очиститель воздуха",
				expected: Inflection{
					Im:   "очиститель воздуха",
					Rod:  "очистителя воздуха",
					Dat:  "очистителю воздуха",
					Vin:  "очиститель воздуха",
					Tvor: "очистителем воздуха",
					Pr:   "очистителе воздуха",
				},
			},
		},
		{
			name: "увлажнитель воздуха",
			input: testCase{
				text: "увлажнитель воздуха",
				expected: Inflection{
					Im:   "увлажнитель воздуха",
					Rod:  "увлажнителя воздуха",
					Dat:  "увлажнителю воздуха",
					Vin:  "увлажнитель воздуха",
					Tvor: "увлажнителем воздуха",
					Pr:   "увлажнителе воздуха",
				},
			},
		},
		{
			name: "колодезный насос",
			input: testCase{
				text: "колодезный насос",
				expected: Inflection{
					Im:   "колодезный насос",
					Rod:  "колодезного насоса",
					Dat:  "колодезному насосу",
					Vin:  "колодезный насос",
					Tvor: "колодезным насосом",
					Pr:   "колодезном насосе",
				},
			},
		},
		{
			name: "лампа слева",
			input: testCase{
				text: "лампа слева",
				expected: Inflection{
					Im:   "лампа слева",
					Rod:  "лампы слева",
					Dat:  "лампе слева",
					Vin:  "лампу слева",
					Tvor: "лампой слева",
					Pr:   "лампе слева",
				},
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, check(tc.input))
	}
}

func TestInflectorClient_InflectWithGrams(t *testing.T) {
	i := Client{}
	actual, err := i.InflectWithHints([]string{"хаб"}, []string{"неод"}, []string{"им", "род", "дат", "вин", "твор", "пр"})

	expected := Inflection{
		Im:   "хаб",
		Rod:  "хаба",
		Dat:  "хабу",
		Vin:  "хаб",
		Tvor: "хабом",
		Pr:   "хабе",
	}

	assert.Equal(t, expected, actual)
	assert.NoError(t, err)
}
