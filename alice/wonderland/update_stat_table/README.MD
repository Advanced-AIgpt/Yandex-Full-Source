Скрипты считают данные для дашборда регрессии компонентов Алисы https://dash.yandex-team.ru/z5nclzdgqka2v?tab=yp

testpalm-regress-size-ALICEQA-318.py считает объём как сумму выполненных кейсов в релизе. Есть разбивка по компонентам.
Тикет: https://st.yandex-team.ru/ALICEQA-318

accepting-time-by-startrek-ALICEQA-368.py считает время приёмки по трекеру для попавших в прод релизов
Тикет: https://st.yandex-team.ru/ALICEQA-368

regress-time-by-testpalm-ALICEQA-397.py считает время регрессов по тестпалму. Выбросы сглаживаются 
Тикет: https://st.yandex-team.ru/ALICEQA-397

[Deprecated из-за смены источника данных графиков ZBP]
ALICEQA-258.py считает даты выкатки релизов на прод для соспоставления с графиками ZBP. Дашборд https://dash.yandex-team.ru/l603xa16drzoi. В остальных расчётах используются даты создания тикетов, поэтому объединить нельзя.
Тикет: https://st.yandex-team.ru/ALICEQA-258
