# Перенос действующих сценариев

1. [Проанализируйте граф аппхоста](#apphost-analysis).
2. [Обеспечьте совместимость по протобафам Scenario State и Arguments](#proto-compatibility).
3. [Подключите dummy-сценарий](#dummy-scenario).
4. [Переведите сценарий на новый интерфейс](#new-interface).
5. [Проведите рефакторинг кода старого сценария](#scenario-refactor).
6. [Актуализируйте граф аппхоста](#apphost-finalize).
7. [Измените рандомные ответы](#random).

## Проанализируйте граф аппхоста {#apphost-analysis}

* Проанализируйте структуру текущего графа и как её можно привести к структуре Hollywood Framework. Обратитесь за консультацией в чат [Hollywood User Group](https://t.me/joinchat/WLI9XSTsGdIdeek2).
* Добавьте проброс протобафа `hw_selected_scene` между всеми нодами сценария.

## Обеспечьте совместимость по протобафам Scenario State и Arguments {#proto-compatibility}

`ScenarioState` и аргументы `Continue`, `Commit` и `Apply` используют Hollywood Framework для хранения данных с возможностью сохранения сценарных параметров в собственных полях.

Чтобы обеспечить совместимость старого и нового сценариев, добавьте поддержку нового чтения данных:

{% cut "Пример чтения Scenario State для сценария weather" %}

Было:

	```cpp
    TWeatherState state;
    const auto& rawState = runRequest.BaseRequestProto().GetState();
    if (rawState.Is<TWeatherState>() && !runRequest.IsNewSession()) {
        rawState.UnpackTo(&state);
	```

Стало:
  
	```cpp
	#include <alice/hollywood/library/framework/framework_migration.h>
	...
	TWeatherState state;
	  if (!runRequest.IsNewSession()) {
          if (NAlice::NHollywood::ReadScenarioState(runRequest.BaseRequestProto(), state)) {
		  ...
          }
	  }
	```
{% endcut %}

Метод `ReadScenarioState()` описан в `framework_migration.h` и обеспечивает чтение состояния как напрямую из `BaseRequestProto::State`, так и через Hollywood Framework.

Уберите из кода старого сценария проверки вида `Y_ENSURE(State.UnpackTo<TMyScenarioState>()).` Не следует предполагать, что в стейте реквеста будет именно «родная» структура сценария.

## Подключите dummy-сценарий {#dummy-scenario}

На этом этапе подключите новый фреймворк поверх старого сценария. Непосредственно в работе сценария ничего не меняется.

Зарегистрируйте новый сценарий с таким же именем и такими же нодами аппхоста, как у старого сценария. Количество нод также должно быть одинаковым (см. [Анализ графа аппхоста](#apphost_analys)).

При запуске Голливуда система обнаружит, что у нас есть одинаковые названия для ручек grpc и применит линковку сценариев. После линковки сценариев все вызовы пойдут в новый сценарий `TMyScenario::Dispatch()`. Возврат `TReturnValueDo();` приводит к тому, что активируется старый обработчик.


{% cut "Шаблон для подготовки кода" %}

```cpp
class TMyScenarioFakeScene : public TScene<TMyScenarioSceneArgs> {
public:
    TMyScenarioFakeScene(const TScenario* owner)
        : TScene(owner, "fake")
    {
    }
    TRetMain Main(const TMyScenarioSceneArgs&, const TRunRequest&, TStorage&, const TSource&) const override {
        HW_ERROR("This scene never called");
    }
};

class TMyScenario : public TScenario {
public:
    TMyScenario()
        : TScenario("my_response") // Название сценария должно совпадать со старым
    {
        Register(&TMyScenario::Dispatch);
        RegisterScene<TMyScenarioFakeScene>([this]() {
            RegisterSceneFn(&TMyScenarioFakeScene::Main);
        });
        // Ноды апхоста должны совпадать с названиями нод в старом сценарии
        SetApphostGraph(ScenarioRequest() >> NodeRun() >> NodeMain() >> ScenarioResponse());
    }
private:
    TRetScene Dispatch(const TRunRequest&,
                       const TStorage&,
                       const TSource&) const {
        // Switch to old scenario immediately
        return TReturnValueDo();
    }
};

HW_REGISTER(TMyScenario);
```
{% endcut %}

После этого можно продолжать переводить старый сценарий на Hollywood Framework.

Также можно расширять код в `TMyScenario::Dispatch()`. Если добавить туда логику обработки входящих параметров и выбора сцен, то для новых запросов управление будет получать новый сценарий, а старые запросы будут уходить в старый код.

## Переведите сценарий на новый интерфейс {#new-interface}

Когда старый и новый сценарий слинкованы, появляется возможность рефакторинга старого сценария и использования новых интерфейсов.

В старом сценарии в структуре `TScenarioHandleContext` теперь будет заполнено поле `NewContext`, из которого можно извлечь новые параметры фреймворка:

```cpp
void TMyOldScenario::Do(TScenarioHandleContext& ctx) const {
    // Убеждаемся, что метод Do() вызван через враппер TReturnValueDo() из слинкованного сценария
    Y_ENSURE(ctx.NewContext != nullptr);
    // Получаем указатели на новые структуры данных в пространстве имен NHollywoodFw
    const NHollywoodFw::TRunRequest& runRequest = *(ctx.NewContext->RunRequest);

    ... работаем с runRequest
	}
```

На этом этапе откажитесь от старых интерфейсов NHollywood и начните пользоваться новыми функциями. Сценарий при этом должен продолжать работать, как раньше. Для проверки работоспособности сценария можно пользоваться IT2, EVO тестами и ручным тестированием.

## Проведите рефакторинг кода старого сценария {#scenario-refactor}

После того, как вы закончили перевод старого кода на новые интерфейсы и отказались от классов типа `TScenarioRunRequestWrapper`, разделите код старого сценария на новые функции:

 * [Диспетчер](../scenarios/dispatcher.md)
 * [Функции сцен (сетевые походы и бизнес-логика)](../scenarios/scene.md)
 * [Функции рендера](../scenarios/renderer.md)

На этом этапе меняется только [логика работы генератора случайных чисел](#random). Традиционный способ и Hollywood Framework имеют отличия в логике инициализации генератора случайных чисел, что может повлиять на запросы к генератору случайных чисел `runRequest.System().Random()` и вариативность ответов NLG в макросах `{%chooseline%}` и `{%chooseitem%}`.

При необходимости переканонизируйте IT2 и скорректируйте анализ в EVO тестах.


## Актуализируйте граф аппхоста {#apphost-finalize}

После окончания работ по переводу сценария может потребоваться адаптация графа аппхоста для типовой схемы, принятой во фреймворке.

Здесь рассмотрены вопросы перевода стандартного однонодового графа на принятую во фреймворке двухнодовую схему.
В случае использования других графов аппхоста обратитесь за консультацией в чат [Hollywood User Group](https://t.me/joinchat/WLI9XSTsGdIdeek2).

1. Cценарию со стандартным однонодовым графом [задан](../apphost/interact.md) кастомный граф аппхоста:

	```cpp
	SetApphostGraph(ScenarioRequest >> ТNode("run") >> ScenarioResponse);
	```

	Его нужно превратить в двухнодовый, однако невозможно согласовать выкатку графов и выкатку новых серверов так, чтобы все текущие запросы продолжили нормальную работу. Поэтому добавьте к сценарию новый граф под флагом эксперимента:

	```cpp
	SetApphostGraph(ScenarioRequest() >> ТNodeRun("run") >> ScenarioResponse());
	SetApphostGraph(ScenarioRequest() >> 
                ТNodeRun("run", "my_scenario_2nodegraph") >> 
                TNodeMain("main") >> 
                ScenarioResponse());
    ```

	В аппхостовый граф добавлены два новых узла `/run` и `/main` под экспериментом `my_scenario_2nodegraph`.

	После проведения эксперимента добавьте флаг `my_scenario_2nodegraph` в `flags.json` и выкатить на 100% пользователей.

2. Подготовьте коммит, в котором:

	* В графах аппхоста удаляется однонодовый вариант, остаются только узлы `/run` и `/main`.
	* В настройке сценария выставляется стандартный двухнодовый граф.

	```cpp
	SetApphostGraph(ScenarioRequest() >> ТNodeRun() >> TNodeMain() >> ScenarioResponse());
	```

	Этот коммит можно выкатить на продакшен в любом порядке (сначала графы, потом настройки сценария, или наоборот).

{% note info %}

* В EVO тестах рекомендуется добавить проверки работоспособности сценария как под флагом эксперимента, так и без оного. По окончании выкатки тесты с флагом эксперимента можно удалить.
* При изменении конфигурации графа может произойти изменение в [логике работы генератора случайных чисел](#random). 
* Чтобы корректно переключать поведение сценария, нельзя пользоваться опцией `:0`, которая применяется в традиционном способе. Необходимо полностью удалять флаг эксперимента, так как:
   * Hollywood трактует флаг `"my_experiment":0` как выключенный эксперимент.
   * Apphost трактует флаг `"my_experiment":0` как включенный эксперимент.

{% endnote %}


## Измените рандомные ответы {#random}

При переводе старого сценария на новый фреймворк в IT2/EVO могут измениться ответы NLG из-за изменения логики работы генератора случайных чисел.

В традиционном способе генератор случайных чисел сидировался один раз на входе в `::Do()` при помощи сида меты, имени сценария и имени ноды. В Hollywood Framework генератор случайных чисел сидируется на каждую вызываемую функцию сценария.

| Фаза сценария   | Традиционный способ   | Hollywood Framework |
| --------------- | --------------------- | ------------------- |
| Ручка `run`     | `seed("run")`         | N/A                 |
| - диспетчер     | ...                   | `seed("dispatch")`  |
| - сетевой поход | ...                   | `seed("setupmain")` |
| Ручка `main`    | `seed("main")`        | N/A                 |
| - `main`        | ...                   | `seed("run")`       |
| - `render`      | ...                   | `seed("render")`    |


Чтобы вернуть прежнюю логику поведения:

* Если генератор случайных чисел используется только в бизнес-логике приложения, вызовите метод `SetCustomRandomHash()`:

	```cpp
	TMyScenario::TMyScenario("my_scenario) {
		...
		SetCustomRandomHash(EStageDeclaration::SceneMainRun, "название ручки в старом сценарии, где располагалась бизнес логика");
	}
	```

* Если генератор случайных чисел используется в рендере (например, `choseline`) и **не используется** в бизнес-логике, выставьте seed подобно предыдущему варианту:

	```cpp
	TMyScenario::TMyScenario("my_scenario) {
		...
		SetCustomRandomHash(EStageDeclaration::Render, "название ручки в старом сценарии, где располагался рендерер");
	}
	```

* Если генератор случайных чисел используется и в рендере, и в бизнес-логике, запретите смену seed в рендере при помощи вызова:

	```cpp
	TMyScenario::TMyScenario("my_scenario) {
		...
		DisableCustomRandomHash(EStageDeclaration::Render);
	}
	```

