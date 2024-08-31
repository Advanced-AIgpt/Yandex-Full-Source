# Логирование событий

Самый прошаренный аналитик [alextim27](https://staff.yandex-team.ru/alextim27)

Для того, чтобы отслеживать поведение пользователей и оценивать, например эксперименты, мы навешиваем счетчики на разные события. В яндексе логирование есть через баобаб и яндекс метрику

## В коде
1. [Баобаб](https://wiki.yandex-team.ru/baobab/)
Отлаживать счетчики в приложении можно при помощи флага `validate_counters=1` и нажатию на бублик (черный круг в правом нижнем приложении)

Пример ссылки: https://uslugi.yandex.ru/?exp_flags=validate_counters=1

   ```js
   // Нода
   export const PriceInput = withBaobab<PriceInputProps>({
       name: 'input',
       attrs: {
           type: 'price',
       },
       events: {
           onClick: logClick(),
       },
   })(PriceInputPresenter);
   ```
   ```js
   // Доопределение
   logNode={{
       name: 'all',
       attrs: { type: 'WorkerCard-AllServices' },
   }}
   ```
1. Яндекс метрика
- Основной счётчик: [49540177](https://metrika.yandex.ru/dashboard?id=49540177)
- [Настройка целей](https://metrika.yandex.ru/49540177?tab=goals)

В коде компонентов используйте функции:
1. Для отправки целей: `ymReachGoal('НАЗВАНИЕ ЦЕЛИ', params?)`. Цель необходимо добавить в интерфейс Метрики, иначе статистика НЕ БУДЕТ считаться.
2. Для параметров визита: `ymParams(params)`.

Доступ к счётчику можно запросить через https://idm.yandex-team.ru, роль: Метрика, доступ к счётчику сервиса Яндекса, номер счётчика: 49540177.

   ```js
   // Цель
   ym('reachGoal', 'ORDER_INVITE_BUTTON', {
       from: 'order_recommendation',
       worker_id: id,
   });
   ```
   ```js
   // Переход
   ym('hit', url);
   ```
## Как посмотреть какие данные отправляются
1. **Баобаб**: Чтобы посмотреть что логируем в Баобаб нужно воспользоваться [baobabviewer](https://jing.yandex-team.ru/files/apanichkina/Снимок%20экрана%202021-02-17%20в%2012.14.34.png).
1. **Яндекс метрика**: Добавив гет-параметр **_ym_debug=1**, можно в консоли увидеть что отправится в метрику.

## Смотрим какие данные дошли
1. **Баобаб**:
   * В [базе](https://yt.yandex-team.ru/hahn/navigation?path=//home/geo-search/alextim27/uslugi/production/squeeze/service&) за прошлые сутки и ранее
   * В [режиме реального времени](https://rtmr.yandex-team.ru/production/data/read/uslugi_redir_log%2F19700101%2Fuser_sessions%2Fraw%2Fyandex_staff?key=y3036662821573579726) (только в инпуте после "y" надо свой yandexuid подставить из cookie)
1. **Яндекс метрика**: [Счетчик](https://h.yandex-team.ru/?https%3A%2F%2Fmetrika.yandex.ru%2Fdashboard%3Fid%3D49540177)
