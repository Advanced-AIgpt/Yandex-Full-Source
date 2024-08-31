# Введение

В разделе про [единый репозиторий](../src/arcadia.md) мы уже упоминали про то, что использование такого репозитория в том числе включает в себя единые правила хранения и оформления кода. В этой части документации собраны основные сведения и правила, касающиеся написания кода.

## Разрешённые языки программирования { #allowed-languages }

Важным ограничением единого репозитория является выбор языка программирования. Зачем это нужно:

* **Командам разработки** подобное ограничение позволяет быть всегда уверенным в том, что на любой проект можно сравнительно легко найти нужное количество опытных разработчиков. Это становится невозможно, если писать на экзотических или слишком новых языках программирования.
* **Команде разработки единого репозитория** это позволяет быстро обеспечивать глубокую интеграцию инструментов разработки с разрешенными языками программирования.

В настоящий момент в едином репозитории разрешены к использованию следующие языки программирования:

Язык | Область применения
:--- | :---
[Bash](https://www.gnu.org/software/bash/) | Небольшие скрипты
[С++](https://isocpp.org/) | Backend, базы данных, map reduce, встраиваемые системы, нативные мобильные приложения
[Go](https://golang.org/) | Backend, консольные утилиты, devops-инструменты
[Java](https://www.java.com) | Backend, мобильные приложения под Android
[JavaScript](https://en.wikipedia.org/wiki/JavaScript) | Frontend. Везде, где возможно, рекомендуется TypeScript.
[Kotlin](https://kotlinlang.org/) | Backend, мобильные приложения под Android.
[Objective C](https://en.wikipedia.org/wiki/Objective-C) | Давно разрабатываемые мобильные приложения под iOS
[Python 3](https://www.python.org/) | Backend, аналитические задачи, задачи машинного обучения. Python 2 считается устаревшим.
[R](https://www.r-project.org/) | Аналитические расчеты
[Swift](https://swift.org/) | Мобильные приложения под iOS
[TypeScript](https://www.typescriptlang.org/) | Frontend. Нельзя писать backend, если это явно не разрешено для вашего проекта.

## Комитеты разработки { #committee }

Ни для кого не секрет, что в разработке программного обеспечения постоянно что-то меняется: появляются новые стандарты, библиотеки и практики. Правила разработки в компании также должны постоянно вбирать в себя лучшие нововведения из внешнего мира. Для того, чтобы иметь возможность подстраивать правила разработки под изменения окружающего мира, в компании введены комитеты разработки. **Комитет разработки** - группа из опытных разработчиков, представляющих разные команды Яндекса. Комитеты отвечают за:

* Определение общих правил разработки для конкретной технологии;
* Утверждение общих компонентов, библиотек и практик, разрешённых к использованию;
* Выработку рекомендаций по сборке, пакетированию, выкладке кода и т.д.;
* Анализ проблем, улучшение жизни и повышение эффективности работы разработчиков.

Правила создаются и меняются по следующей схеме:

1. Возникает вопрос для регулирования;
2. Комитет изучает текущую ситуацию;
3. Комитет принимает согласованное решение и при необходимости выносит его на публичное обсуждение;
4. Комитет собирает обратную связь и при необходимости устраивает рабочие встречи с командами;
5. Комитет публикует окончательное решение в настоящей документации и в клубе в Этушке.

Если вы хотите связаться с комитетом лично, то основной способ это сделать - написать на почтовую рассылку. Адреса рассылок приведены ниже.

Комитеты проводят **еженедельные встречи**, на которые регулярно приходят гости. Если у вас есть вопрос, требующий обсуждения с комитетом, прежде всего нужно заранее написать на рассылку комитета. Если вопрос требует устного обсуждения, вас обязательно пригласят на встречу комитета и у всех участников будет время на подготовку. Иногда комитеты сами приглашают к себе гостей обсудить какой-либо вопрос.

Особо важные вопросы могут обсуждаться на **открытых встречах** с комитетом в формате доклад / ответы на вопросы. Темы таких встреч утверждаются заранее. Если у вас есть тема, которую хочется обсудить в формате открытой встречи, - напишите на почтовую рассылку комитета.

Существует набор профильных комитетов по конкретным технологиям. Над профильными комитетами находится мета-комитет. Он утверждает решения профильных комитетов, решает вопросы, не вошедшие ни в один профильный комитет и разрешает любые конфликты или вопросы, которые не смог разрешить профильный комитет.

### Мета-комитет { #meta-committee }
Состав: [Антон Самохвалов](https://staff.yandex-team.ru/pg), [Алексей Башкеев](https://staff.yandex-team.ru/abash), [Андрей Стыскин](https://staff.yandex-team.ru/styskin) (привлекается при необходимости)

Публикации: [Development in Arcadia](https://clubs.at.yandex-team.ru/arcadia/), [Журнал про Яндекс](https://clubs.at.yandex-team.ru/mag/)

Приватно задать вопрос: пишите [Олег Смоляков](https://staff.yandex-team.ru/saint) или участникам комитета.

### C++ комитет { #cpp-committee }
Состав: [C++ комитет](https://abc.yandex-team.ru/services/cppcommittee/)

Встречи: [Calendar](https://calendar.yandex-team.ru/event/58943088)

Публикации: [Development in Arcadia](https://clubs.at.yandex-team.ru/arcadia/posts.xml?tag=43771)

Задать вопрос по почте: [cpp-com@](mailto:cpp-com@yandex-team.ru)

Очередь задач: [CPPCOM](https://st.yandex-team.ru/CPPCOM)

Правила разработки: [C++](https://docs.yandex-team.ru/arcadia-cpp/)

### Go комитет { #go-committee }
Состав: [Go комитет](https://abc.yandex-team.ru/services/committeego/)

Публикации: [Golang](https://clubs.at.yandex-team.ru/golang/posts.xml?tag=35649)

Задать вопрос по почте: [go-com@](mailto:go-com@yandex-team.ru)

Очередь задач: [GOCOM](https://st.yandex-team.ru/GOCOM)

Правила разработки: [Go](https://wiki.yandex-team.ru/devrules/Go/)

### Frontend комитет { #frontend-committee }
Состав: [Frontend комитет](https://abc.yandex-team.ru/services/frontend-committee/)

Публикации: [Verstka](https://clubs.at.yandex-team.ru/verstka/posts.xml?tag=33532)

Задать вопрос по почте: [front-com@](mailto:front-com@yandex-team.ru)

Очередь задач: [FRONTCOM](https://st.yandex-team.ru/FRONTCOM)

Правила разработки: [Frontend](https://wiki.yandex-team.ru/devrules/Frontend/)

### Java комитет { #java-committee }
Состав: [Java комитет](https://abc.yandex-team.ru/services/committeejava/)

Публикации: [Java](https://clubs.at.yandex-team.ru/java/posts.xml?tag=32853), [Development in Arcadia](https://clubs.at.yandex-team.ru/arcadia/posts.xml?tag=35646)

Задать вопрос по почте: [java-com@](mailto:java-com@yandex-team.ru)

Очередь задач: [JAVACOM](https://st.yandex-team.ru/JAVACOM)

Правила разработки: [Java](https://docs.yandex-team.ru/arcadia-java/)

### Python комитет { #python-committee }
Состав: [Python комитет](https://abc.yandex-team.ru/services/committeepython/)

Публикации: [Python](https://clubs.at.yandex-team.ru/python/posts.xml?tag=32936)

Задать вопрос по почте: [python-com@](mailto:python-com@yandex-team.ru)

Очередь задач: [PYTHONCOM](https://st.yandex-team.ru/PYTHONCOM)

Правила разработки: [Python](https://docs.yandex-team.ru/arcadia-python/)

### Комитет по мобильной разработке { #mobile-committee }
Состав: [Комитет по мобильной разработке](https://abc.yandex-team.ru/services/mobilecommitee/)

Публикации: [mobile-dev](https://clubs.at.yandex-team.ru/mobile-dev/)

Задать вопрос по почте: [mobile-com@](mailto:mobile-com@yandex-team.ru)

Очередь задач: [MOBILECOM](https://st.yandex-team.ru/MOBILECOM)

### Комитет по данным { #data-committee }
Работает над стандартизацией передачи, изменения, записи и хранения данных.

Состав: [Комитет по данным](https://abc.yandex-team.ru/services/data-com/)

Задать вопрос по почте: [data-com@](mailto:data-com@yandex-team.ru)

### Рабочая группа по общим вопросам единого репозитория { #arcadia-wg }
Состав: [Arcadia Working Group](https://abc.yandex-team.ru/services/arcadia-wg/)

Публикации: [Development in Arcadia](https://clubs.at.yandex-team.ru/arcadia/posts.xml?tag=43772)

Задать вопрос по почте: [arcadia-wg@](mailto:arcadia-wg@yandex-team.ru)

### Комитет ML { #ml-committee }
Состав: [Комитет ML](https://abc.yandex-team.ru/services/mlcommittee/)

Публикации: [https://vag-ekaterina.at.yandex-team.ru/298](https://vag-ekaterina.at.yandex-team.ru/298)

### Комитет по закрытым репозиториям { #private-repo-committee }
Состав: [Антон Самохвалов](https://staff.yandex-team.ru/pg), [Сергей Певцов](https://staff.yandex-team.ru/spev), [Олег Смоляков](https://staff.yandex-team.ru/saint), [Эльдар Заитов](https://staff.yandex-team.ru/ezaitov)

Публикации: [https://clubs.at.yandex-team.ru/mag/47863](https://clubs.at.yandex-team.ru/mag/47863)
