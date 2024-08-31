# О Detekt

## [Описание](https://github.com/detekt/detekt)

## Как запустить локально

Через команду `./gradlew detekt`

## Подсветка в студии

Поставь плагин `detekt`. После этого в студии появится подсветка проблемных мест

## Как писать кастомные правила

### Статьи о написании

[Habr](https://habr.com/ru/company/citymobil/blog/565402/)
[Github](https://detekt.github.io/detekt/extensions.html#custom-rulesets)
[Medium](https://medium.com/@vanniktech/writing-your-first-detekt-rule-ee940e56428d)

### В нашем проекте

В модуле `tools` -> `custom-detekt-rules`

По аналогии с другими рулами пишем свой + тест на него

### Как разобраться с API

`PSI` не самый простой, но благо есть плагин `PsiViewer`, который поможет понять, какие поля использовать для своего правила
[Статья](https://itnext.io/write-custom-android-kotlin-linting-rules-like-a-psi-chic-e081e032da2f) про этот плагин

### Как запустить

После написания нужно:
- добавить правило в `detekt-config.yaml`
- запустить скрипт `try-custom-detekt-rules.sh`

### Baseline

Если не хотите после написания нового правила править во всём проекте замечания, можно обновить `baseline`, запустив команду `./gradlew detektBaseline`
