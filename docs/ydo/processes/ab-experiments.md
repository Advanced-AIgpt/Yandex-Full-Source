# AB эксперименты
* Ликбез по флагам в [доке](https://docviewer.yandex-team.ru/view/1120000000041217/?page=4&*=m%2FJCgzoi%2Bl7cgWZut8G6HftZkJJ7InVybCI6InlhLXdpa2k6Ly93aWtpLWFwaS55YW5kZXgtdGVhbS5ydS95ZG8vbmV3c3RhZmYvZmxhZ3MucGRmIiwidGl0bGUiOiJmbGFncy5wZGYiLCJub2lmcmFtZSI6ZmFsc2UsInVpZCI6IjExMjAwMDAwMDAwNDEyMTciLCJ0cyI6MTY1MjI3NTY5MjU2MCwieXUiOiIzODY3OTE1MjE2NDA2MDMzNjcifQ%3D%3D)

Как посмотреть экспенименты? Вот [тут](https://wiki.yandex-team.ru/sbs/manual/search/pluginsfiltersflags/sbs-flags/) + залипнуть

Почти все изменения на фронте мы выкатываем через AB эксперименты, ниже описание жизненного цикла новой функциональности

![text](../_assets/life-cicle.png "Жизненный цикл функциональности" =550x140)

1. Заводим функциональность под флагом. Для того, чтобы проанализировать эксперимент, необходимо добавить логирование, дока [тут](../common-tech/analytics.md)
2. Релизим задачу разработки
3. Менеджер или разработчик заводит эксперимент в AB. [Пример экспа](https://ab.yandex-team.ru/task/EXPERIMENTS-58878) , где в [выборках](https://ab.yandex-team.ru/testid/304857) задается нужная комбинация из флагов, добавленных в п1
4. Эксп проверяется и если все ок выкатывается на нужный процент как правило на неделю
5. После завершения экспа аналитики или менеджер смотрят результаты и принимают решение о выкатке на 100%
6. Эксперимент катится на 100% через [flags.json](https://wiki.yandex-team.ru/serp/report/man/flags.json/)

# Как сделать функционал под флагом
1. Заводим файл с описанием флага по аналогии с остальными файлами в директории [expflags](https://github.yandex-team.ru/search-interfaces/frontend/tree/master/services/ydo/expflags) или командой `npx expflags`
2. В компонентах используем селектор [isExpFlagEnabled](https://a.yandex-team.ru/arc_vcs/frontend/services/ydo/src/store/experiment/selectors.ts?rev=9956c10dd652f909af0c3fbe6a043c68c6cc8201#L34), `getExpFlagNumberValue` или `getExpFlagValue` в зависимости от типа значения флага и под условием показываем экспериментальный функционал
3. Пишем код готовый к раскатке в любой момент, так как при раскатке флага мы не рефакторим то, что было написано

# Как быстро раскатить флаг через flags.json
0. Делается это в специальной таске, [пример](https://st.yandex-team.ru/YDO-5671), которая генерится автоматом
1. Добавляем значение флага в [конфиг](https://a.yandex-team.ru/arc/trunk/arcadia/frontend/services/ydo/.config/app/testExpFlags.ts) с учетом платформы
2. Переснимаем упавшие Hermione тесты

**Не делаем никаких продуктовых изменений**

# Как раскатить флаг в коде
0. Раскатка делается в специальной таске (чтобы автоматика удаляла флаг из flags.json)
1. Удаляем файл с флагом из `expflags`
2. Удаляем флаг из [конфига](https://a.yandex-team.ru/arc/trunk/arcadia/frontend/services/ydo/.config/app/testExpFlags.ts)
3. Переснимаем тесты, где была завязка на нужный exp_flags. Если тесты лежали в директории /experiments, то выносим их на [общий уровень](https://jing.yandex-team.ru/files/apanichkina/Снимок%20экрана%202021-02-03%20в%2014.33.57.png)

# Как оторвать(удалить) флаг из кода
в обратном порядке см. Добавление функционала под флагом

# Как заводить эксперименты
* Видео в двух частях от нашего аналитика
* [Часть 1](https://jing.yandex-team.ru/files/apanichkina/expflags_1.mp4)
* [Часть 2](https://jing.yandex-team.ru/files/apanichkina/expflags_21.mp4)
* [Преза](https://jing.yandex-team.ru/files/apanichkina/Эксперименты%20в%20услугах.pptx)
