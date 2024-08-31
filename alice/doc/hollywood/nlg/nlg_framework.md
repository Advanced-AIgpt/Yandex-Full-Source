# Изменения по NLG-рендерам

Если ваш сценарий ранее использовал NLG-описание в предыдущих версиях Голливуда (или даже в VINS), то учтите ключевые изменения, которые произошли в Hollywood Framework. При работе с рендером Hollywood Framework ориентируется на передачу данных через протобафы.

## Изменения в naming convension

В старых вариантах NLG использовались переменные вида `Context.some_variables`.
Так как в протобафах члены классов объявляются в CamelUpperCase, то адресоваться к этим переменным надо соответствующим образом:

```
protobuf:
    String SomeVariable;

nlg:
    not_var{{ Context.SomeVariable }}
```

## Optional и отсутствующие поля

В силу специфики работы конвертера Protobuf → NLG Context, все отсутствующие поля будут декларированы со значением `0`/`false`/`""`. Проверки типа `if defined Context.Variable` более не имеют смысла, и их необходимо переписать на новые. Например, `if Context.Variable != ""`.

## Сложные переменные

Протобаф правильно работает только с переменными простых типов (строки, числа). Однако периодически в NLG надо передавать сложные объекты, например, `date.time` или `city.preparse`. Передавайте эти переменные в контекст при помощи дополнительного метода `Render::MakeComplexVar()`:

1. Объявите в протобафе переменную нужного имени (например, `CityPreparse`).
2. На этапе подготовки данных для рендера запишите в эту функцию JSON-строку с соответствующим содержимым.
3. На этапе работы рендера предварительно вызовите метод `Render::MakeComplexVar()`.

{% cut "Пример для sys.datetime" %}
```cpp
// Протобаф:
string SourceDate;

// Код сцены по подготовке данных
renderArgs.SetSourceDate(JsonToString(dateParsed.GetAsJsonDatetime()));

// Код рендера
render.MakeComplexVar("SourceDate", NJson::ReadJsonFastTree(renderArgs.GetSourceDate()));
```
{% endcut %}

{% cut "Пример для city.preparse" %}
```cpp
// Протобаф:
string CityPreparse;

// Код сцены по подготовке данных
NSc::TValue caseForms;
NAlice::AddAllCaseForms(geobase, id, TStringBuf("ru"), &caseForms, /* wantObsolete = */ true);
renderArgs.SetCityPreparse(caseForms.ToJsonSafe());

// Код рендера
render.MakeComplexVar("CityPreparse", NJson::ReadJsonFastTree(args.GetCityPreparse()));
```
{% endcut %}

## Дальнейшее развитие системы рендера

В планах расширить компилятор NLG таким образом, что протобаф рендера импортировался первой строчкой в NLG-файле. В случае, если протобаф найден, компилятор NLG переходит в режим строгого соответствия, когда для любой записи `Context.Var` переменная с именем Var должна существовать в описании протобафа.
