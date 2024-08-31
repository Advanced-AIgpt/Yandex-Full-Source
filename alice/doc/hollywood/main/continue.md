# Continue/Apply/Commit

Фреймворк поддерживает сценарии с дополнительными ручками `/continue` `/apply` и `/commit`.
Чтобы поддержать дополнительные ручки, надо:

* Создать дополнительный граф аппхоста для обработки новой ручки;
* Сконфигурировать в конструкторе сценария новый граф при помощи метода SetApphostGraph();
* Дополнить вашу сцену новыми функциями для обработки;
* Вернуть новый вариант кода возврата из Main().

## Дополнительный граф аппхоста для `/continue` `/apply` или `/commit`.

Создается при помощи скрипта генератора сценария или стандартным способом через [конфигурацию аппхоста](https://a.yandex-team.ru/arc_vcs/apphost/conf/verticals/ALICE/).
Рекомендованные названия нод графа:

{% list tabs %}

- Continue

  * `continue_setup` - для сценария с ручкой `/continue` и с опциональными сетевыми походами
  * `continue` - для основной бизнес логики `/continue`

- Apply

  * `apply_setup` - для сценария с ручкой `/apply` и с опциональными сетевыми походами
  * `apply` - для основной бизнес логики `/apply`

- Commit

  * `commit_setup` - для сценария с ручкой `/commit` и с опциональными сетевыми походами
  * `commit` - для основной бизнес логики `/commit`

{% endlist %}

## Регистрация аппхостового графа

Для поддержки нового графа аппхоста зарегистрируйте его в конструкторе сценария:

{% list tabs %}

- Continue

  ```cpp
  SetApphostGraph(ScenarioContinue() >> NodeContinueSetup() >> NodeContinue() >> ScenarioResponse());
  ```
  В случае, если имена ваших нод отличаются от типовых (`continue_setup`/`continue`), то эти имена надо указать первым аргументом у функций `NodeXxx()`.

- Apply

  ```cpp
  SetApphostGraph(ScenarioApply() >> NodeApplySetup() >> NodeApply() >> ScenarioResponse());
  ```
  В случае, если имена ваших нод отличаются от типовых (`apply_setup`/`apply`), то эти имена надо указать первым аргументом у функций `NodeXxx()`.

- Commit

  ```cpp
  SetApphostGraph(ScenarioCommit() >> NodeCommitSetup() >> NodeCommit() >> ScenarioResponse());
  ```
  В случае, если имена ваших нод отличаются от типовых (`commit_setup`/`commit`), то эти имена надо указать первым аргументом у функций `NodeXxx()`.

{% endlist %}

## Функции сцены для обработки ручек /continue /apply /commit

Фреймворк обрабатывает пришедшие ручки `/continue` `/apply` и `/commit` сразу в той сцене, которая была выбрана в диспетчере на этапе `/run`.
Чтобы получить управление, надо зарегистрировать дополнительные функции и добавить в диспетчер граф аппхоста для обработки ручки.

{% list tabs %}

- Continue

  1. Объявите в вашей сцене следующие функции:
  ```cpp
  // Сетевой поход из /continue, эта функция по аналогии с MainSetup() тоже может отсутствовать
  TRetSetup ContinueSetup(const MySceneArgs&, const TContinueRequest& request, const TStorage& storage) const override;
  // Основная обработка ручки /continue
  TRetContinue Continue(const MySceneArgs& sceneArgs, const TContinueRequest& request, TStorage& storage, const TSource& source) const override;
  ```

  * Параметр `sceneArgs` - это аргументы сцены, точно такие же, как были в функции `Main()`;
  * Параметр `request` типа `TContinueRequest` содержит
    * метод `TContinueRequest::GetArguments()` для извлечения аргументов, заданных в `TReturnValueContinue`;
    * базовый класс `TRequest`, заполненный точно также, как и в функции `Main()`;
  * Поле `source` содержит ответы источников, которые были заданы в `ContinueSetup()`. Если функция `ContinueSetup()` отсутствует или не настраивала походы в источники, это будет пустая структура;
  * Поле `storage` используется для доступа к состоянию сценария или мементо;
  * Возвратом из функции `Continue()` может быть `TReturnValueRender()`, `TReturnValueDo()` или `TError`.

  2. Зарегистрируйте их (её) внутри `RegisterScene()` по аналогии с функцией `Main()`:
  ```cpp
  RegisterSceneFn(&TMyScene::ContinueSetup); // Опционально, если эта функция есть
  RegisterSceneFn(&TMyScene::Continue);
  ```

- Apply

  1. Объявите в вашей сцене следующие функции:
  ```cpp
  // Сетевой поход из /apply, эта функция по аналогии с MainSetup() тоже может отсутствовать
  TRetSetup ApplySetup(const MySceneArgs&, const TApplyRequest& request, const TStorage& storage) const override;
  // Основная обработка ручки /apply
  TRetContinue Apply(const MySceneArgs& sceneArgs, const TApplyRequest& request, TStorage& storage, const TSource& source) const override;
  ```

  * Параметр `sceneArgs` - это аргументы сцены, точно такие же, как были в функции `Main()`;
  * Параметр `request` типа `TApplyRequest` содержит
    * метод `TApplyRequest::GetArguments()` для извлечения аргументов, заданных в `TReturnValueApply`;
    * базовый класс `TRequest`, заполненный точно также, как и в функции `Main()`;
  * Поле `source` содержит ответы источников, которые были заданы в `ApplySetup()`. Если функция `ApplySetup()` отсутствует или не настраивала походы в источники, это будет пустая структура;
  * Поле `storage` используется для доступа к состоянию сценария или мементо;
  * Возвратом из функции `Apply()` может быть `TReturnValueRender()`, `TReturnValueDo()` или `TError`.

  2. Зарегистрируйте их (её) внутри `RegisterScene()` по аналогии с функцией `Main()`:
  ```cpp
  RegisterSceneFn(&TMyScene::ApplySetup); // Опционально, если эта функция есть
  RegisterSceneFn(&TMyScene::Apply);
  ```

- Commit

  1. Объявите в вашей сцене следующие функции:
  ```cpp
  // Сетевой поход из /commit, эта функция по аналогии с MainSetup() тоже может отсутствовать
  TRetSetup CommitSetup(const MySceneArgs&, const TCommitRequest& request, const TStorage& storage) const override;
  // Основная обработка ручки /commit
  TRetCommit Commit(const MySceneArgs& sceneArgs, const TCommitRequest& request, TStorage& storage, const TSource& source) const override;
  ```

  * Параметр `sceneArgs` - это аргументы сцены, точно такие же, как были в функции `Main()`;
  * Параметр `request` типа `TCommitRequest` содержит
    * метод `TCommitRequest::GetArguments()` для извлечения аргументов, заданных в `TReturnValueCommit`;
    * базовый класс `TRequest`, заполненный точно также, как и в функции `Main()`;
  * Поле `source` содержит ответы источников, которые были заданы в `CommitSetup()`. Если функция `CommitSetup()` отсутствует или не настраивала походы в источники, это будет пустая структура;
  * Поле `storage` используется для доступа к состоянию сценария или мементо;
  * Возвратом из функции `Commit()` может быть `TReturnValueSuccess()` или `TError`.

  2. Зарегистрируйте их (её) внутри `RegisterScene()` по аналогии с функцией `Main()`:
  ```cpp
  RegisterSceneFn(&TMyScene::CommitSetup); // Опционально, если эта функция есть
  RegisterSceneFn(&TMyScene::Commit);
  ```

{% endlist %}

## Возврат управления из Main()

{% list tabs %}

- Continue

  Для того, чтобы пойти в ручку `/continue`, из Main()-функции сцены надо вернуть код возврата:

  ```cpp
  return TReturnValueContinue(const MyProto& continueArgs, TRunFeatures features = TRunFeatures{});
  ```

  Где
  * `continueArgs` - протобаф с аргументами, которые придут в ручку `/continue`.
  * `features` - опциональный класс с инструкциями для Megamind для работы постклассификатора `/run`.

- Apply

  Для того, чтобы пойти в ручку `/apply`, из Main()-функции сцены надо вернуть код возврата:

  ```cpp
  return TReturnValueApply(const MyProto& applyArgs, TRunFeatures features = TRunFeatures{});
  ```

  Где
  * `applyArgs` - протобаф с аргументами, которые придут в ручку `/apply`.
  * `features` - опциональный класс с инструкциями для Megamind для работы постклассификатора `/run`.

- Commit

  Для того, чтобы пойти в ручку `/commit`, из Main()-функции сцены надо вернуть код возврата:

  ```cpp
  return TReturnValueCommit(&MyRenderFn, const MyRenderArgs& renderArgs, const MyProto& commitArgs, TRunFeatures features = TRunFeatures{});
  ```

  Где
  * `MyRenderFn` - указатель на функцию рендера
  * `renderArgs` - аргументы для рендера
  * `applyArgs` - протобаф с аргументами, которые придут в ручку `/commit`.
  * `features` - опциональный класс с инструкциями для Megamind для работы постклассификатора `/run`.

{% endlist %}

## Важные замечания

* Функции сцены в ручках `/continue` `/apply` и `/commit` получают **те же самые аргументы**, что и в `/run`. Те данные, которые вы передаете в `TReturnValueContinue()`, можно получить из `TContinueRequest::GetArguments()`!
* Если вы заполните `TStorage` в функции Main(), пойдете в ручку `/continue` или `/apply`, но не выиграете в постклассификаторе, сделанные изменения в TStorage будут потеряны.
* Поле `source` в функции `Continue()` НЕ СОДЕРЖИТ ответов источников, которые были получены в ноде `/run`
* Установка TRunFeatures в функции `Continue()` невозможна.
* Функции `Continue()` и `Apply()` завершаются вызовом функции рендера (как и в `Main()`). При возврате из `Main()` результата `TReturnValueCommit()` рендер будет вызван сразу после этого. Поэтому из функции `Commit()` можно вернуть либо `TReturnValueSuccess`, либо `TError`.
