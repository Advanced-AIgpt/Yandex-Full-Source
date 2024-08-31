### Идея

Автоматизация работы с релизами и прилинкованными к ним тикетами.

Автозакрытие протестированных тикетов после выкатки релизов в прод: https://st.yandex-team.ru/ALICEQA-130
Обновление текущей стадии для багов not_showstopper https://st.yandex-team.ru/ALICEQA-177
Автозакрытие прод-релизов, когда выкатывается новый https://st.yandex-team.ru/ALICEQA-201

Работает с очередями 'ALICERELEASE', 'ALICE', 'DIALOG', 'ASSISTANT', 'MEGAMIND', 'IOT', 'QUASARUI', 'PASKILLS', 'CENTAUR'

### Логика
1. Сверяем компоненты очереди с захардкоженным списком. Если появится новый компонент, мы получим предупреждение.
2. Проверяем, не выкатилось ли нового релиза той же компоненты. Если да, закрываем устаревшее.
3. В чатик Q отправляется сообщение с результатом

1. Спрашиваем количество актуальных релизов в статусе Production
2. Запускаем цикл по связанным с релизами тикетам.
3. Для каждого прилинкованного тикета проверяем состояние: очередь, статус=протестировано, тег not_showstopper, оба Stage
4. Если протестировано - закрываем
5. Если нестоппер & не протестирован & оба стейджа Release - меняем текущую стадию на прод
6. В чатик Q отправляется сообщение с результатом
...
PROFIT

Сначала срабатывает close_old_production_releases.py, потом close_tickets_of_production_release.py
Скрипт close_tickets_of_closed_release.py больше не нужен, шедулер с ним отключён.