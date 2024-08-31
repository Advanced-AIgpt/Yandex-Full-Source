package dbmigration

import (
	"sort"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

func TestBuildDeclareFromFields(t *testing.T) {
	ttable := []struct {
		name   string
		fields ydbFields
		res    string
	}{
		{
			name:   "empty",
			fields: ydbFields{},
			res:    "",
		},
		{
			name:   "one",
			fields: ydbFields{{Name: "$a", Value: ydb.OptionalValue(ydb.Uint64Value(1))}},
			res:    "DECLARE $a AS Optional<UInt64>;\n",
		},
		{
			name: "two",
			fields: ydbFields{
				{Name: "$first", Value: ydb.OptionalValue(ydb.Uint64Value(0))},
				{Name: "$second", Value: ydb.OptionalValue(ydb.StringValueFromString(""))},
			},
			res: "DECLARE $first AS Optional<UInt64>;\nDECLARE $second AS Optional<String>;\n",
		},
	}

	for _, test := range ttable {
		t.Run(test.name, func(t *testing.T) {
			at := assert.New(t)
			res, err := buildDeclareFromFields(test.fields)
			at.NoError(err)
			at.Equal(test.res, res)
		})
	}
}

func TestBuildQueryParams(t *testing.T) {
	ttable := []struct {
		name   string
		orig   *table.QueryParameters
		append ydbFields
		res    *table.QueryParameters
	}{
		{
			name:   "empty",
			orig:   table.NewQueryParameters(),
			append: ydbFields{},
			res:    table.NewQueryParameters(),
		},
		{
			name:   "without_add",
			orig:   table.NewQueryParameters(table.ValueParam("$a", ydb.BoolValue(true))),
			append: ydbFields{},
			res:    table.NewQueryParameters(table.ValueParam("$a", ydb.BoolValue(true))),
		},
		{
			name:   "add_to_empty",
			orig:   table.NewQueryParameters(),
			append: ydbFields{{Name: "$f", Value: ydb.Int64Value(1)}},
			res:    table.NewQueryParameters(table.ValueParam("$f", ydb.Int64Value(1))),
		},
		{
			name: "usual",
			orig: table.NewQueryParameters(
				table.ValueParam("$first", ydb.BoolValue(true)),
				table.ValueParam("$second", ydb.StringValueFromString("asd")),
			),
			append: ydbFields{
				{Name: "$append_one", Value: ydb.Int64Value(1)},
				{Name: "$append_second", Value: ydb.StringValueFromString("ddd")},
			},
			res: table.NewQueryParameters(
				table.ValueParam("$first", ydb.BoolValue(true)),
				table.ValueParam("$second", ydb.StringValueFromString("asd")),
				table.ValueParam("$append_one", ydb.Int64Value(1)),
				table.ValueParam("$append_second", ydb.StringValueFromString("ddd")),
			),
		},
	}

	for _, test := range ttable {
		t.Run(test.name, func(t *testing.T) {
			at := assert.New(t)
			res, err := buildQueryParams(test.orig, test.append)
			at.NoError(err)

			var resultValues ydbFields
			res.Each(func(name string, value ydb.Value) {
				resultValues = append(resultValues, ydbField{
					Name:  name,
					Value: value,
				})
			})

			var testResultValues ydbFields
			test.res.Each(func(name string, value ydb.Value) {
				testResultValues = append(testResultValues, ydbField{
					Name:  name,
					Value: value,
				})
			})

			sort.Slice(resultValues, func(i, j int) bool {
				return resultValues[i].Name < resultValues[j].Name
			})
			sort.Slice(testResultValues, func(i, j int) bool {
				return testResultValues[i].Name < testResultValues[j].Name
			})

			at.Len(resultValues, len(testResultValues))
			for i := range testResultValues {
				at.Equal(testResultValues[i].Name, resultValues[i].Name)
				cmp, err := ydb.Compare(resultValues[i].Value, testResultValues[i].Value)
				at.NoError(err, testResultValues[i].Name)
				at.Zero(cmp, testResultValues[i].Name)
			}
		})
	}
}

func TestBuildRangeWhereFromFields(t *testing.T) {
	ttable := []struct {
		name       string
		fieldNames []string
		res        string
	}{
		{
			name:       "empty",
			fieldNames: []string{},
			res:        "",
		},
		{
			name:       "one",
			fieldNames: []string{"first"},
			res:        "(first > $first)",
		},
		{
			name:       "two",
			fieldNames: []string{"first", "second"},
			res:        "(first > $first) OR (first == $first AND second > $second)",
		},
		{
			name:       "three",
			fieldNames: []string{"first", "second", "third"},
			res:        "(first > $first) OR (first == $first AND second > $second) OR (first == $first AND second == $second AND third > $third)",
		},
	}

	for _, test := range ttable {
		t.Run(test.name, func(t *testing.T) {
			at := assert.New(t)
			res := buildRangeWhereFromFields(test.fieldNames)
			at.Equal(test.res, res)
		})
	}
}

func TestYdbTypeName(t *testing.T) {
	ttable := []struct {
		name string
		v    ydb.Value
		res  string
	}{
		{
			name: "string",
			v:    ydb.StringValueFromString(""),
			res:  "Optional<String>",
		},
	}

	for _, test := range ttable {
		t.Run(test.name, func(t *testing.T) {
			at := assert.New(t)
			res, err := ydbTypeName(test.v)
			at.NoError(err)
			at.Equal(test.res, res)
		})
	}
}
