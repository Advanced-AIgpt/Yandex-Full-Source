# Форматирование Go кода

Минимальное требование - [gofmt](https://golang.org/cmd/gofmt).

Настоятельно рекомендуется использование [goimports](https://pkg.go.dev/golang.org/x/tools/cmd/goimports) с аргументом `-local a.yandex-team.ru`. Руками запускается как `goimports -w -local a.yandex-team.ru <path>`. В GoLand настраивается через File watchers -> tools -> goimports с нужными аргументами.

Самый хороший вариант - [yoimports](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yoimports). От `goimports` отличается тем что всегда сортирует импорты в 3 группы - stdlib, внешние и аркадийные. Руками запускается как `ya tool yoimports -w <path>`. В GoLand настраивается через File watchers -> tools -> ya tool yoimports с нужными аргументами.
Или, можно при настройке проекта воспользоваться утилитой `ya` с ключем `--with-yoimports`:
```
ya ide goland --with-yoimports
```
