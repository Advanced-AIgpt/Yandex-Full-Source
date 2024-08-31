package dbmigration

import (
	"fmt"
	"strings"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ydbField struct {
	Name  string
	Value ydb.Value
}

type ydbFields []ydbField

func buildDeclareFromFields(fields ydbFields) (string, error) {
	buf := &strings.Builder{}
	for _, field := range fields {
		typeName, err := ydbTypeName(field.Value)
		if err != nil {
			return "", xerrors.Errorf("failed to detect type of field %q: %w", field.Name, err)
		}
		buf.WriteString(fmt.Sprintf("DECLARE %v AS %v;\n", field.Name, typeName))
	}
	return buf.String(), nil
}

func buildQueryParams(originalParams *table.QueryParameters, fields ydbFields) (*table.QueryParameters, error) {
	res := table.NewQueryParameters()

	originalParams.Each(func(name string, value ydb.Value) {
		res.Add(table.ValueParam(name, value))
	})

	for _, field := range fields {
		res.Add(table.ValueParam(field.Name, field.Value))
	}
	return res, nil
}

func buildRangeWhereFromFields(fieldNames []string) string {
	var alternatives []string
	for morePosition, moreFieldName := range fieldNames {
		andParts := make([]string, 0, morePosition)

		for _, equalFiendName := range fieldNames[:morePosition] {
			andParts = append(andParts, fmt.Sprintf("%s == %s", equalFiendName, varName(equalFiendName)))
		}

		andParts = append(andParts, fmt.Sprintf("%s > %s", moreFieldName, varName(moreFieldName)))
		alternatives = append(alternatives, "("+strings.Join(andParts, " AND ")+")")
	}

	return strings.Join(alternatives, " OR ")
}

func toYdbFields(fieldNames []string, fieldValues []ydb.Value) (ydbFields, error) {
	if len(fieldNames) != len(fieldValues) {
		return nil, xerrors.Errorf("failed to create ydbFields: len(fieldNames) != len(fieldValues). Names: %v", fieldNames)
	}
	res := make(ydbFields, 0, len(fieldNames))
	for i := range fieldNames {
		res = append(res, ydbField{Name: "$" + fieldNames[i], Value: fieldValues[i]})
	}
	return res, nil
}

func varName(fieldName string) string {
	return "$" + fieldName
}

var ydbTypeNames = map[string]ydb.Value{
	"Optional<String>":    ydb.OptionalValue(ydb.StringValueFromString("")),
	"Optional<UInt64>":    ydb.OptionalValue(ydb.Uint64Value(0)),
	"Optional<Timestamp>": ydb.OptionalValue(ydb.TimestampValue(0)),
	"Optional<Bool>":      ydb.OptionalValue(ydb.BoolValue(false)),
}

func ydbTypeName(v ydb.Value) (string, error) {
	for name, ydbExample := range ydbTypeNames {
		if _, err := ydb.Compare(v, ydbExample); err == nil {
			return name, nil
		}
	}

	return "", xerrors.Errorf("failed to detect type for: %+v", v)
}
