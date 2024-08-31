# Поиск TSemanticFrames и TSlots

С изменениями от 20.10.2021

## Саммари

Для поиска semantic frames и конкретных слотов используются методы `TScenarioInputWrapper::FindSemanticFrame()` и `TFrame::FindSlot()`. Эти методы возвращали сырые указатели, что при ошибках кода могло привести к необработанному access violation. Для предотвращения этого поведения сигнатуры были изменены на

```
// Было
const NAlice::TSemanticFrame* TScenarioInputWrapper::FindSemanticFrame(const TStringBuf frameName) const;
// Стало
const TPtrWrapper<NAlice::TSemanticFrame> TScenarioInputWrapper::FindSemanticFrame(const TStringBuf frameName) const;

// Было
TSlot* TFrame::FindSlot(const TStringBuf name) const;

// Стало
TPtrWrapper<TSlot> TFrame::FindSlot(const TStringBuf name) const;
```

Класс TPtrWrapper<> описан [тут](https://a.yandex-team.ru/arc_vcs/alice/hollywood/library/util/tptrwrapper.h) и является аналогом std::reference_wrapper с рядом исключений:

* дает только константный доступ
* бросает исключение с названием проблемного фрейма, если доступ к объекту произошел без предварительной проверки на nullptr

Примеры использования:

```
// было
const auto* mySlot = frame->FindSlot("my_slot_name");
// стало
const auto mySlot = frame->FindSlot("my_slot_name");
...
// Старый вариант в этом месте бросит segmentation fault, если mySlot не был проверен на nullptr
// Новый вариант кинет исключение с диагностикой "Trying to access to null pointer for object 'my_slot_name'"
if (mySlot->Value) { 
    ...
    
// Проверять фрейм или слот на nullptr можно разными способами:
if (mySlot)
if (mySlot != nullptr)
if (!mySlot.IsValid())
```

Доступ к мемеберам TSemanticFrame и TSlot с использованием враппера не изменяется. В ряде редких случаев может потребоваться вызвать ручное разыменование mySlot.Get() для получения константного указателя.
