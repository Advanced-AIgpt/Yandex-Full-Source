//
// Random number, based on NEW hollywood framework
//

// Основной подкчаемый файл с декларациями главного диспетчера
#include "random_number.h"
// Описание обработчика сцены "загадай случайное число"
// Каждую из сцен можно удалить из сценария
#include "random_number_scene_rnd.h"
// Описание обработчика сцены "брось кубик"
// Каждую из сцен можно удалить из сценария
#include "random_number_scene_dice.h"

// Автогенерируемый файл от NLG random_number
#include <alice/hollywood/library/scenarios/random_number/nlg/register.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/json/json.h>

namespace NAlice::NHollywoodFw::NRandomNumber {

namespace {

//
// Подготовка аргументов сцены для "Загадай случайное число"
//
TRandomNumberSceneArgsRandom CreateSceneArgsRandom(const TRunRequest& runRequest, const TFrameRandomNumber& fr) {
    TRandomNumberSceneArgsRandom state;
    if (fr.LowerBound.Defined()) {
        state.SetLowerBound(*fr.LowerBound);
    }
    if (fr.UpperBound.Defined()) {
        state.SetUpperBound(*fr.UpperBound);
    }
    LOG_DEBUG(runRequest.Debug().Logger()) << "Input data: " << JsonStringFromProto(state);
    return std::move(state);
}

//
// Подготовка аргументов сцены для "Брось кубик"
//
TRandomNumberSceneArgsDice CreateSceneArgsDice(const TRunRequest& runRequest, const TFrameThrowDice& fd) {
    TRandomNumberSceneArgsDice state;
    state.SetMainFrame(true);
    if (fd.NumEdges.Defined()) {
        state.SetEdgeCount(*fd.NumEdges);
    }
    if (fd.NumThrows.Defined()) {
        state.SetThrowCount(*fd.NumThrows);
    }
    if (fd.GameCode.Defined()) {
        state.SetGameCode(*fd.GameCode);
    }
    // Поле num_dices может придти в слоте несколько раз ('два' и 'кубики').
    // Проверяем все их, выбираем максимальное по значению
    state.SetDiceCount(1);
    for (const auto it : fd.NumDices.Value) {
        if (it <= 0) {
            // Невалидные варианты значений будут обработаны в сцене
            state.SetDiceCount(0);
            break;
        }
        state.SetDiceCount(Max(state.GetDiceCount(), it));
    }
    LOG_DEBUG(runRequest.Debug().Logger()) << "Input data: " << JsonStringFromProto(state);
    return std::move(state);
}

} // anonimous namespace


/*
    Регистрация сценария в системе
*/
HW_REGISTER(TRandomNumberScenario);

/*
    Базовый конструктор для сценария RANDOM_NUMBER
    - Инициализирует базовый класс сценария TScenario
    - Регистрирует основные функции (в нашем случае только Dispatch)
    - Регистрирует основные обработчики (в нашем случае только 1 обработчик)
    - Регистрирует рендерер
    - Вызывает вспомогательные функции для настройки работы сценария
*/
TRandomNumberScenario::TRandomNumberScenario()
    : TScenario(NProductScenarios::RANDOM_NUMBER)
{
    // Регистрируем главную точку входа (диспетчер сценария)
    Register(&TRandomNumberScenario::Dispatch);
    // Регистрируем Scene. В нашем случае - это 2 обработчика - для запросов "назови случайное число" и "брось кубик".
    RegisterScene<TRandomNumberSceneRandom>([this]() {
        // Регистрируем функции бизнес-логики. В нашем случае это только одна тривиальная функция Main()
        RegisterSceneFn(&TRandomNumberSceneRandom::Main);
    });
    RegisterScene<TRandomNumberSceneDice>([this]() {
        // Аналогично для второго сценария
        RegisterSceneFn(&TRandomNumberSceneDice::Main);
    });

    // Регистрируем рендереры.
    // Дефолтовый рендерер иррелевантного запроса
    RegisterRenderer(&TRandomNumberScenario::RenderIrrelevant);

    // Дополнительные функции
    // Регистрация NLG для генерации текста
    SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NRandomNumber::NNlg::RegisterAll);
    // Значение product scenario name для системы аналитики
    SetProductScenarioName("random_number");
    // (Опционально) Возможные значения протобафов сценарного стейта

    // (Опционально) Названия семантических фреймов, которые разрешены для получения в сценарий
    AddSemanticFrame(FRAME_NAME_DICE);
    AddSemanticFrame(FRAME_NAME_DICE_MORE);
    AddSemanticFrame(FRAME_NAME_RANDOM);

    // Установка двунодового графа (этот код необязателен, этот граф дефолтный и добавится автоматически, оставлен для примера)
    SetApphostGraph(ScenarioRequest() >> NodeRun() >> NodeMain() >> ScenarioResponse());

    // Этот код требуется только для согласования поведения генератора случайных чисел со старым продакшен сценарием
    DisableCustomRandomHash(EStageDeclaration::Render);
}

/*
    Базовый диспетчер сценария
    Получает от системы первый обработчик ручки "RUN".
    Если существует SF "alice.random_number" - передает управление
    Если существует SF "alice.throw_dice" - передает управление
    Если существует SF "alice.throw_dice.more" - передает управление
    В противном случае возвращает Irrelevant
*/
TRetScene TRandomNumberScenario::Dispatch(const TRunRequest& runRequest,
                                          const TStorage&,
                                          const TSource&) const
{
    //
    // Проверки для сцены "загадай случайное число"
    //
    TFrameRandomNumber fr(runRequest.Input());
    if (fr.Defined()) {
        // Semantic Frame для "Загадай случайное число" найден
        return TReturnValueScene<TRandomNumberSceneRandom>(CreateSceneArgsRandom(runRequest, fr), fr.GetName());
    }

    //
    // Проверки для сцены "брось кубик"
    //
    // Сначала проверяем FRAME_NAME_DICE_MORE
    if (runRequest.Input().HasSemanticFrame(FRAME_NAME_DICE_MORE)) {
        // Стандартная бизнес логика, все данные возьмутся из Storage, при условии, что запрос норм
        LOG_DEBUG(runRequest.Debug().Logger()) << "Throw dice 'more' frame found, using scenario state";
        // В этом возврате используется TString(FRAME_NAME_DICE_MORE) так как TFrame не был создан
        return TReturnValueScene<TRandomNumberSceneDice>(TRandomNumberSceneArgsDice{}, TString(FRAME_NAME_DICE_MORE));
    }
    // Теперь проверяем стандартный фрейм
    TFrameThrowDice fd(runRequest.Input());
    if (fd.Defined()) {
        // Semantic Frame для "Брось кубик" найден и эксперимент включен
        LOG_DEBUG(runRequest.Debug().Logger()) << "Throw dice frame found, using standard processing";
        return TReturnValueScene<TRandomNumberSceneDice>(CreateSceneArgsDice(runRequest, fd), fd.GetName());
    }

    // Semantic Frame / Experiments не найдены, сценарий нерелевантен
    LOG_ERR(runRequest.Debug().Logger()) << "Semantic frames not found";

    // В данном примере используется функция иррелевантного рендерера, которая является частью сценария
    return TReturnValueRenderIrrelevant(&TRandomNumberScenario::RenderIrrelevant, {});
    // Также можно воспользоваться готовой функцией фреймворка:
    // return TReturnValueRenderIrrelevant("random_number", "error");
}

/*
    TRandomNumberScenario::RenderIrrelevant()

    Рендерер иррелевантного ответа для сценария
    Используется в диспетчере и функциях сцен, если во время обработки запросов произошли ошибки

    Обратите внимание, что сценарии не выставляют SetIrrelevant() явным образом
    Единственный способ выставить иррелевантный ответ - -это
        return TReturnValueIrrelevant{&TRandomNumberScenario::RenderIrrelevant, {}};
*/
TRetResponse TRandomNumberScenario::RenderIrrelevant(const TRandomNumberRenderIrrelevant&, TRender& render) {
    auto& logger = render.GetRequest().Debug().Logger();
    LOG_DEBUG(logger) << "Irrelevant answer from dispatcher";
    render.CreateFromNlg("random_number", "error", NJson::TJsonValue{});
    return TReturnValueSuccess();
}

} // namespace NAlice::NHollywoodFw::NRandomNumber
