# Кодогенерация

## Введение

Hollywood framework использует специальные механизмы кодогенерации для построения врапперов над протобафами.

В традиционном варианте Hollywood недостаточно было задекларировать новые поля протобафов, надо было еще руками дописать С++ код во врапперах, которые помогают узнавать исходные параметры запроса (как например с `TInterfaces`) или помогают построить ответ (в `TRunResponseBuilder`).
В новом Hollywood Framework используется автоматическая кодогенерация.

В настоящее время поддержаны механизмы кодогенерации для следующих протобафов:

* [TDirectives](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/directives.proto)

## Как работает кодогенерация

Кодогенерация основана на дополнительном инструментарии [jinja2_compiler](https://a.yandex-team.ru/arc/trunk/arcadia/alice/tools/jinja2_compiler).
Он компилирует файл протобафа, а затем преобразует его в кастомный cpp/h код, используя язык разметки [Jinja2](https://jinja2docs.readthedocs.io/en/stable/).

Более подробно о кодогенерации можно прочитать в описании [кодогенератора](https://a.yandex-team.ru/arc/trunk/arcadia/alice/tools/jinja2_compiler/readme.md).

Для кодогенерации иcпользуются специальные шаблоны в папке hollywood/library/framework/core/codegen. Они преобразуются в файлы cpp/h.
Для того, чтобы запустить кодогенерацию, воспользуйтесь файлом run.sh, который обновит соответствующие cpp/h файлы.

{% note warning %}

В отличии от генерируемых файлов протобафов `pb.h` дополнительные кодогенеренные файлы имеют нетривиальную структуру, поэтому сохраняются непосредственно в репозиторий Аркадии.
Если вы изменили один из протобафов, то вам может потребоваться закоммитить не только сам протобаф, но и измененные в результате кодогенерации cpp/h файлы.

{% endnote %}

В ряде случаев для правильной работы кодогенерации требуется наличие дополнительных MessageOptions или FieldOptions. Детали будут описаны ниже.

## Кодогенерация во фреймворке

### TDirectives

Исходные файлы:

* arcadia/alice/hollywood/library/framework/core/codegen/directives.h.jinja2 
* arcadia/alice/hollywood/library/framework/core/codegen/directives.cpp.jinja2 

Выходные файлы: 

* arcadia/alice/hollywood/library/framework/core/codegen/directives.h
* arcadia/alice/hollywood/library/framework/core/codegen/directives.cpp

Файлы являются враппером для класса TDirective и позволяют добавлять директивы в итоговый Response.
Эти методы доступны в основном интерфейсе фреймворка в классе `TRender` через дополнительный метод `TRender::Directives()`.

Кодогенерация создает 1 или 2 варианта добавления директивы
* для простых директив (есть только одно поле `Name`):
  Используйте метод `render.Directives().AddXxxx()` без параметров (`Xxxx` - название вашей директивы);
* для сложных директив:
  Используйте метод `render.Directives().AddXxxx(Xxxx&& directive)`.
  Вам надо предварительно объявить и заполнить требуемую директиву.
