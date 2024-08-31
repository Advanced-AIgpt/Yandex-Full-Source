## Полезные инструменты

## Инструмент для определения фрейма в SuccessCondition-е и кнопке постролла {#tool_frame_grep}
[Граф в нирване](https://nirvana.yandex-team.ru/flow/45c902c9-1d58-4f0e-88b4-3b4b6474a318/0c507da8-4e78-46b5-932f-34f76ee3e323/graph), помогающий выбрать SuccessCondition и TypedSemanticFrame у нового постролла.

Глобальные опции:
* ```query_cnt``` - максимальное количество удовлетворяющих regexp'ам запросов. При отсутствии установленного значения ставится 10000000000.
* ```query_regexp```, ```intent_regexp```, ```scenario_regexp``` - регулярки на текст запроса, интента и сценария соответственно. 

Выходы:
* ```frame_table``` - таблица с уникальными фреймами для SuccessCondition и вхождениями соответствующих фреймов в рассматриваемый датасет.
* ```top_frame_proto``` - прото представление фрейма с самым большим количеством вхождений в рассматриваемый датасет. TypedSematicFrame является примером фрейма для кнопки, Name и Slots являются примером для SuccessCondition.

Пример: при ```query_regexp``` = ```утреннее шоу```, ```intent_regexp``` = ```.*```, ```scenario_regexp``` = ```.*``` получим следующий ```top_frame_proto```:
```bash
Name: "alice.alice_show.activate"
Slots {
  Name: "day_part"
  Value: "Morning"
}
TypedSemanticFrame {
  AliceShowActivateSemanticFrame {
    DayPart {
      DayPartValue: Morning
    }
  }
}
```

Тогда один из вариантов постролла будет таким:
```bash
Items {
    Id: "my_morning_show_with_button_example"
    BaseItem: "my_morning_show_with_button"
    Result: {
        Postroll: {
            Voice: "Кстати, я могу включить вам утреннее шоу. Включить?"
        }
        FrameAction: {    // описание кнопки
            ParsedUtterance: {
              TypedSemanticFrame {    // TypedSemanticFrame из top_frame_proto
                AliceShowActivateSemanticFrame {
                  DayPart {
                    DayPartValue: Morning
                  }
                }
              }
              Analytics {
                Purpose: "play_morning_show"
              }
            }
        }
    }
    Analytics: {
        SuccessConditions {
            Frame {     // фрейм из top_frame_proto
                Name: "alice.alice_show.activate"
                Slots {
                  Name: "day_part"
                  Value: "Morning"
                }
            }
        }
    }
}
```