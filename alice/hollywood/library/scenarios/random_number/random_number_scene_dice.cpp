#include "random_number_scene_dice.h"

#include <alice/hollywood/library/scenarios/random_number/proto/random_number_fastdata.pb.h>

namespace NAlice::NHollywoodFw::NRandomNumber {

namespace {

//
// Константы по умолчанию для "Брось кубик"
//
constexpr int DEFAULT_EDGE_COUNT = 6;

constexpr int MAX_DICE_COUNT = 10;
constexpr int MAX_EDGE_COUNT = 120;

//
// FastData для "Брось кубик" (каталог игр и количество/типы кубиков для них)
//
class TRandomNumberFastData : public IFastData {
public:
    TRandomNumberFastData(const TThrowDiceFastDataProto& proto) {
        for (const auto& it : proto.GetGameDefinitions()) {
            GameList_[it.GetGameCode()] = TGameDefinition{it.GetDiceCount(), it.GetEdgeCount()};
        }
    }
    bool FindGame(const TString& gameCode, int& diceCount, int& edgeCount) const {
        const auto gamePtr = GameList_.FindPtr(gameCode);
        if (gamePtr == nullptr) {
            return false;
        }
        diceCount = gamePtr->DiceCount_;
        edgeCount = gamePtr->EdgeCount_;
        return true;
    }
private:
    struct TGameDefinition {
        int DiceCount_ = 0;
        int EdgeCount_ = 0;
    };
    TMap<TString, TGameDefinition> GameList_;
};

} // Anonimous namespace

/*
    Конструктор регистрации для TRandomNumberSceneDice
*/
TRandomNumberSceneDice::TRandomNumberSceneDice(const TScenario* owner)
    : TScene(owner, SCENE_NAME_DICE)
{
    // Загрузка фастдаты
    AddFastData<TThrowDiceFastDataProto, TRandomNumberFastData>("random_number/random_number.pb");

    // Регистрируем рендереры - брось кубик...
    RegisterRenderer(&TRandomNumberSceneDice::RenderDice);
}

/*
    Главная логика работы TRandomNumberSceneDice (функция Main())
*/
TRetMain TRandomNumberSceneDice::Main(const TRandomNumberSceneArgsDice& args,
                                      const TRunRequest& runRequest,
                                      TStorage& storage,
                                      const TSource&) const
{
    if (args.GetMainFrame() && (args.GetDiceCount() < 1 || args.GetDiceCount() > MAX_DICE_COUNT)) {
        return TReturnValueRender("random_number", "wrong_dice_count", args);
    }

    // Извлекаем данные из TStorage
    TRandomNumberScenarioState scenarioState;
    bool isNewSession = false;
    switch (storage.GetScenarioState(scenarioState, std::chrono::seconds(300))) {
        case TStorage::EScenarioStateResult::Absent:
        case TStorage::EScenarioStateResult::Expired:
        case TStorage::EScenarioStateResult::NewSession:
            // Данные сценария не существуют или прошло слишком много времени
            // Или новая сессия (решение тут за сценарием)
            // Контроль времени задан в SetScenarioStateTimeout(std::chrono::seconds(300));
            LOG_DEBUG(runRequest.Debug().Logger()) << "Session is expired, clear state";
            scenarioState.Clear();
            isNewSession = true;
            break;
        case TStorage::EScenarioStateResult::Present:
            // Будем использовать данные сценария
            break;
    }
    // Добавляем / переписываем значения из args
    if (args.GetMainFrame() && args.GetDiceCount() > 0) {
        scenarioState.SetDiceCount(args.GetDiceCount());
    }
    if (args.GetMainFrame() && args.HasEdgeCount()) {
        scenarioState.SetEdgeCount(args.GetEdgeCount());
    }
    // Проверяем код игры
    if (!args.GetGameCode().Empty()) {
        LOG_INFO(runRequest.Debug().Logger()) << "Game name request: " << args.GetGameCode();
        const auto& fd = runRequest.System().GetFastData().GetFastData<TRandomNumberFastData>();
        int diceCount = 0;
        int edgeCount = 0;
        if (fd && fd->FindGame(args.GetGameCode(), diceCount, edgeCount) && diceCount && edgeCount) {
            LOG_INFO(runRequest.Debug().Logger()) << "Game name override: dices: " << diceCount << "; edges: " << edgeCount;
            scenarioState.SetDiceCount(diceCount);
            scenarioState.SetEdgeCount(edgeCount);
        } else {
            // Я не знаю такую игру
            LOG_ERROR(runRequest.Debug().Logger()) << "Undefined game name: " << args.GetGameCode();
            return TReturnValueRender("random_number", "dont_know_game", scenarioState);
        }
    }
    if (!scenarioState.HasEdgeCount()) {
        scenarioState.SetEdgeCount(DEFAULT_EDGE_COUNT);
    }

    TRandomNumberRenderStateDice rst;
    rst.SetNewSession(isNewSession);
    rst.SetDiceCount(scenarioState.GetDiceCount());
    rst.SetEdgeCount(scenarioState.GetEdgeCount());

    LOG_DEBUG(runRequest.Debug().Logger()) << "ROLL_DICE: Dices: " << scenarioState.GetDiceCount() <<
              "; Edges: " << scenarioState.GetEdgeCount();

    if (scenarioState.GetDiceCount() < 1 || scenarioState.GetDiceCount() > MAX_DICE_COUNT) {
        return TReturnValueRender("random_number", "wrong_dice_count", rst);
    }
    if (scenarioState.GetEdgeCount() < 1 || scenarioState.GetEdgeCount() > MAX_EDGE_COUNT) {
        return TReturnValueRender("random_number", "to_many_edges", rst);
    }

    // Сохраняем ScenarioState обратно в TStorage
    storage.SetScenarioState(scenarioState);

    // Основная бизнес-логика сценария + готовим данные для ответа
    int sum = 0;
    for (int i = 0; i < scenarioState.GetDiceCount(); i++) {
        int result = runRequest.System().Random().RandomInteger(1, scenarioState.GetEdgeCount() + 1);
        LOG_INFO(runRequest.Debug().Logger()) << "ROLL_DICE: Result: " << result;
        rst.AddValues(result);
        sum += result;
    }
    rst.SetSum(sum);
    return TReturnValueRender(&TRandomNumberSceneDice::RenderDice, rst);
}

/*
    Рендерер для "брось кубик"
*/
TRetResponse TRandomNumberSceneDice::RenderDice(const TRandomNumberRenderStateDice& state, TRender& render) {
    render.CreateFromNlg("random_number", "dice_result", state);
    render.AddEllipsisFrame(FRAME_NAME_DICE_MORE);
    return TReturnValueSuccess();
}

} // namespace NAlice::NHollywoodFw::NRandomNumber
