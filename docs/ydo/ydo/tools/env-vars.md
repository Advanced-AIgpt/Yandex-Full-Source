# Переменные окружения

### При сборке приложения
1. `HERMIONE_RUN` - (тип: boolean) включение режима сборки для прогона hermione тестов. Влияет на выбор конфигурации приложения.
2. `YENV` - (тип: enum, значения: `development | testing | production` ) режим сборки. Влияет на выбор конфигурации приложения, минификацию кода и способа загрузки статики.
3. `WEBPACK_FULL_STATS` - (тип: boolean) включить полный набор stats при сборке. Нужно для анализа состава бандлов.
4. `DISABLE_MODULE_CONCATENATION` - (тип: boolean) отменить конкатенацию модулей в stats. Нужно для анализа состава бандлов.
5. `YPLATFORM` - (тип: enum, значения: `desktop | touch`) исключительный выбор платформы для сборки клиентского кода. В комбинации с `npm start` позволяет экономить на сборке, разрабатывая только на одной платформе.
6. `BUILD_ALL_ICONS` - (тип: boolean) включить сборку всех иконок (favicons для разных браузеров) приложения. Влияет на скорость серверной сборки. По умолчанию при `YENV=development` собираются только базовые иконки + иконки для Android. При `YENV=testing|production` собираются все иконки.
7. `DISABLE_SSR` - (тип: boolean) отключает Server Side Rendering (используется при npm run start). Ускоряет сборку (собирается меньше кода, используется более быстрый esbuild-loader).
8. `PROFILE_WEBPACK` - (тип: enum, значения: `server | desktop | touch | service-worker`) добавляет speed-measure-webpack-plugin и включает профилирование в webpack.

### При сборке Storybook
Собрать/запустить/задеплоить Storybook для определённого компонента/папки можно с помощью переменных окружения `YDO_STORYBOOK_FOLDER` и `YDO_STORYBOOK_FILE`.

Например:
```bash
YDO_STORYBOOK_FOLDER=./src/features/ui-kit/address npm run storybook:build

YDO_STORYBOOK_FILE='Address.story.tsx' npm run storybook:build
```
