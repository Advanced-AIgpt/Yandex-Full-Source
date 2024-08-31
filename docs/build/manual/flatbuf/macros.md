# Flatbuffers : макросы

Макросы для сборки Flatbuffers:
- [SRCS](#srcs)
- [EXCLUDE_TAGS](#exclude_tags)
- [FBS_NAMESPACE](#fbs_namespace)
- [FLATC_FLAGS](#flatc_flags)
- [INCLUDE_TAGS](#include_tags)
- [ONLY_TAGS](#only_tags)

## SRCS
В макросе `SRCS` перечисляются все `.fbs` файлы необходимые для сборки.

## EXCLUDE_TAGS
```EXCLUDE_TAGS(Tags)```

Вызов макроса `EXCLUDE_TAGS` позволяет отключить инстанциацию подмодулей [FBS_LIBRARY](modules.md#fbs_library) для указанных вариантов (тэгов, перечисленных в аргументах макроса). По умолчанию инстанциируются следующие варианты [FBS_LIBRARY](modules.md#fbs_library): `CPP_FBS`, `GO_FBS`, `JAVA_FBS`, `PY2_FBS`, `PY3_FBS`.

## FBS_NAMESPACE
```FBS_NAMESPACE(Name.space)```

В макросе `FBS_NAMESPACE` нужно указать `Flatbuffers` namespace, используемый в этом модуле.

{% note info %}

Использование этого макроса действительно требуется только в зависимых `FBS_LIBRARY` (транзитивно) и если нужна сборка для `Go`. Требование вытекает из особенностей сборки `Flatbuffers` для `Go`.

{% endnote %}

{% note info %}

Ограничения на использование `Flatbuffers` в `Аркадийной` сборке - в сборке модуля может быть определён только один `Flatbuffers` namespace.

{% endnote %}

## FLATC_FLAGS
```FLATC_FLAGS(Flags)```

В макросе `FLATC_FLAGS` можно указать дополнительные флаги для `flatc` в текущем модуле.

## INCLUDE_TAGS
```INCLUDE_TAGS(Tags)```

Вызов макроса `INCLUDE_TAGS` позволяет добавить инстанциацию подмодулей [FBS_LIBRARY](modules.md#fbs_library) для указанных вариантов (тэгов, перечисленных в аргументах макроса).

## ONLY_TAGS
```ONLY_TAGS(Tags)```

Вызов макроса `ONLY_TAGS` переопределяет набор инстанциаций подмодулей [FBS_LIBRARY](modules.md#fbs_library) для указанных вариантов (тэгов, перечисленных в аргументах макроса).
