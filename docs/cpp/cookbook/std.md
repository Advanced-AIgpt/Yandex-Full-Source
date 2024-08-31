# Hitchhiker's Guide to C++ in Arcadia: Compilers, Runtime, Standard Library

![](../img/dont_panic.png)

## Статическая сборка

По умолчанию мы используем (почти) статическую сборку для исполняемых файлов на С++.

Это означает, что в любом коммите мы **можем** сломать [ABI](https://en.wikipedia.org/wiki/Application_binary_interface) в любой части репозитория (если поведение функции или класса меняются обратно несовместимым образом, автор коммита **должен** исправить всех клиентов — это можно сделать в том же коммите или в более ранних).

Возможность сломать ABI в произвольный момент времени делает паттерн программирования [PImpl](https://en.wikipedia.org/wiki/Opaque_pointer) избыточным (паттерн всё ещё можно использовать для оптимизации времени сборки). По тем же самым причинам атрибут `[[deprecated]]` вместо предупреждения генерирует ошибку сборки: вместо пометки метода устаревшим, его нужно удалить и перевести клиентов на новый метод.

{% note warning %}

glibc линкуется к вашей программе динамически (glibc вообще [плохо поддерживает](https://stackoverflow.com/a/57478728) статическую сборку). Такой тип линковки делает собранную в Аркадии программу зависимой от версии glibc, доступной в момент запуска.

Подробности про совместимость между различными версиями libc описаны в следующем разделе.

{% endnote %}

## Стандартная библиотека C (libc)

Используемая в момент сборки glibc задаётся переменной сборки `OS_SDK` (по умолчанию значению соответствует _Ubuntu 12.04 Precise Pangolin_). Попытка запустить собранную программу на предыдущих версиях Ubuntu (например, на _10.04 Lucid Lynx_) обречена на провал: загрузчик не сможет найти некоторые библиотечные функции и программа не запустится.

Для решения этой проблемы существуют следующие опции:

* Использовать библиотеку [musl](http://musl.libc.org/) вместо glibc. Собранная таким образом программа зависит только от ядра Linux.

    Проверить собираемость вашего проекта с musl можно с помощью команды `ya make --musl`.

* Дописать недостающие символы в библиотеку совместимости [libc_compat](https://arcanum.yandex-team.ru/arc/trunk/arcadia/contrib/libs/libc_compat).

    Эта библиотека позволяет уравнять API libc из разных операционных систем; в частности, она содержит реализацию некоторых функций libc под Windows.

    Библиотека наполняется функциями совместимости on demand и пытается уровнять все существующие платформы между собой.

## Cтандартная библиотека C++

Мы используем [libc++](https://libcxx.llvm.org/) в качестве нашей стандартной библиотеки.
По умолчанию компилятором С++ является clang (на Linux и MacOS) или MSVC (на Windows).

{% note warning %}

Замена компилятора на MSVC **не приводит** к использованию нами [стандартной библиотеки от Microsoft](https://github.com/microsoft/STL).
Мы добились того, чтобы libc++ можно было (почти полностью) собрать при помощи компилятора MSVC.

{% endnote %}

Часть функций не реализованы, так как не имеют поддержки в апстриме:

* libc++ не содержит реализации функций from_chars из `<charconv>` для типов `float` и `double`. Вместо них можно использовать написанные нами функции `FromString()` из `<util/string/cast.h>`.

Кроме описанных выше особенностей, в нашей стандартной библиотеке сделаны следующие заметные снаружи изменения:

* Отключена упаковка для `std::vector<bool>`: каждый элемент такого вектора занимает 1 байт.
* Для `std::basic_string` добавлен метод `resize_uninitialized(size_t new_size)`, изменяющий размер строки без заполнения её символами `'\0'`.
* Для `std::vector<Pod>` добавлен метод `resize_uninitialized(size_t new_size)`, изменяющий размер контейнера без выполнения конструкторов по умолчанию для аллоцированных вновь элементов.
* Для типов `std::vector` и `std::basic_string` (но не для `std::basic_string_view`) итераторы сделаны обычными указателями.

{% note info %}

Описанные выше изменения применяются, только если флаг препроцессора `_YNDX_LIBCPP_ENABLE_EXTENSIONS` выставлен в `1` (он выставлен по умолчанию).

Проверить собираемость вашего кода с классической стандартной библиотекой можно с помощью команды `ya make -DCFLAGS=-D_YNDX_LIBCPP_ENABLE_EXTENSIONS=0`.

Кроме этого, почти все расширения снабжены отдельными флагами. Полный список флагов можно найти [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/libs/cxxsupp/libcxx/include/__wrappers_config).

{% endnote %}

## C++-рантайм

Кроме стандартной библиотеки мы используем [libcxxrt](https://github.com/libcxxrt/libcxxrt) для имплементации низкоуровневых функций
(RTTI, `dynamic_cast`, выбрасывание исключений и некоторая другая функциональность языка С++ требует наличия этой — или аналогичной — библиотеки).

Некоторые активно используемые компиляторами механизмы не реализованы в этой библиотеке. Для их реализации мы используем [libcxxabi](https://libcxxabi.llvm.org) из LLVM.
В частности, libcxxabi используется для реализация `thread_local`-переменных.

{% note warning %}

`thread_local`-переменные полностью функциональны, но ведут себя по-разному на разных платформах, так как Стандарт С++ не гарантирует ленивую инициализацию таких переменных.
Согласно [тесту](https://arcanum.yandex-team.ru/arc/trunk/arcadia/contrib/tests/compiler/thread_local) на Linux и Darwin инициализация будет ленивой, а на Windows — нет.
Чтобы получить одинаковое поведение на всех платформах, стоит использовать `Y_THREAD` / `FastTlsSingleton` из `<util/thread/singleton.h>`.
**Эти конструкции не являются полным эквивалентом** `thread_local`; перед применением нужно ознакомиться с [документацией](https://a.yandex-team.ru/arc/trunk/arcadia/util/thread/singleton.h).

**Попытка использования `Y_THREAD`-переменных из `std::thread` приведёт к утечкам под Windows**.
Для корректной работы такие переменные нужно использовать только вместе с [TThread](https://arcanum.yandex-team.ru/arc/trunk/arcadia/util/system/thread.h).

{% endnote %}

На Windows C++-рантайм берётся из Windows SDK.
На некоторых мобильных платформах могут использоваться другие имплементации C++-рантайма.