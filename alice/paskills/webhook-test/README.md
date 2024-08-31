## Тестовый навык для проверки работы диалогов

### Как запустить локально
1. npm install  
2. npm run build && npm run start  

### Чтобы собрать докер для qloud
npm run docker
https://paskills.priemka.voicetech.yandex.net/test-skill
https://paskills-common-testing.alice.yandex.net/test-skill/
   
### Команды
команда - проверка   
//основные  
0 - обычный ответ  
1 - отсуствует 'session'  
2 - ОШИБКА неверная версия version: '2.0' (true = version: '1.0')  
3 - ОШИБКА отсутствует версия 'version'  
4 - отсуствует 'session_id'  
400 - session_id  
5 - отсуствует 'message_id'  
500 - message_id  
6 - отсуствует 'user_id'  
7 - отсутствует end_session   
8 - ОШИБКА отсутствует 'response.text'  
9 - ОШИБКА отсутствует 'response.text'  
10 - ОШИБКА в поле 'text' более 1024 символа  
11 - отсутствует 'tts'  
12 - ОШИБКА в поле tts более 1024 символа  
// кнопки  
13 - кнопка button  
14 - кнопки buttons  
15 - ОШИБКА отсутствует 'buttons.title'  
16 - в 'buttons.title' больше 64 символов  
17 - отсутствует 'buttons.url'  
18 - отсутствует 'buttons.hide'  
// текст  
пиццу на улицу льва толстого дом 16 - NER  
19 - эмоджи  
🙂 - эмоджи  
20 - интерпретация символов М  
21 - перенос 1  
22 - перенос 2 длинный текст с переносами  
23 - перенос 3  
// саджесты  
24 - саджесты  
241 - саджесты с различными вариантами заполнения  
242 - саджесты со message_id и session_id  
// картинки   
25 - картинка 1 пустая  
26 - картинка 2  
27 - ОШИБКА отсутствует 'card.image_id'  
28 - ОШИБКА большой 'card.image_id'  
29 - отсутствует 'card.type'  
30 - отсутствует 'card.title'  
31 - галлерея картинок  
310 - payload в card  
32 - галерея картинок отсутствует 'items.type'  
33 - галерея картинок отсутствуют 'items.image_id'  
34 - ОШИБКА в 'items' > 5 карточек  
35 - ОШИБКА неправильный 'card.items'  
// звуки и эффекты  
36 - звук 1 эффект голоса  
37 - звук 2 длинный  
38 - звук 3 - 8 бит  
39 - звук 4 + картинка  
40 - звук 5  
// ошибки в JSON  
41 - ошибки в JSON 1  
42 - ошибки в JSON 2  
43 - ошибки в JSON 3  
// end_session  
44 - end_session = true  
45 - end_session = false  
// state  
46 - change session state  
47 - change user state  
48 - change session and user state  
49 - сброс user state  
// player  
50 - директива Play offset_ms:0  
51 - директива Play offset_ms:5000  
52 - директива Stop  
53 - запуск mp3 с end_session
54 - песня с приглушением и end_session
55 - Длинная песня
60 - BigImageList
61 - BigImageList с описанием под картинками
// кастомные ивенты
65 - 2 кастомных ивента
// intents  
эй включи свет в ванной - intents  
число 13 - yandex_type  
// задержки  
часы - задержка 1.9 сек  
2 секунды - задержка 2 сек  
2,5 секунды - задержка 2.5 сек  
1 и 9 секунды - задержка 2.5 сек  
2,8 секунды - задержка 2.8 сек  
3 секунды - задержка 2.9 сек  
3 полных секунды - задержка 3 сек  (таймаут)
//Оплата в навыках  
закажи пиццу - старт оплаты в навыке
закажи пиццу бесплатно - старт тестовой оплаты в навыке(без списания денег)
// авторизация  
авторизация (start_account_linking)  
!!! баланс - нужно сделать  
логин (access_token)  
чек логин  
точка - возвращает точку в text и tts  
включи музыку - возвращает пустой text  
// Пользовательские продукты в навыке  
квест - старт активации продукта  
//Кастомные ивенты
65 - 2 кастомных ивента
66 - пустой value
67 - много текста
68 - 11  уровней вложенности
69 - 2 ивента с одиннаковым name
70 - 2 ивента с одиннаковым name и value 
71 - 2 ивента с одиннаковым name и разными value
72 - ивент с value равным true

//http ошибки  
ошибка http 500 - ответить кодом 500  
ошибка http 400 - ответить кодом 400 (аналогично для любого кода)  

Соц шаринг: https://dialogs.yandex.ru/sharing/doc?image_url=https%3A%2F%2Favatars.mds.yandex.net%2Fget-zen_doc%2F3840910%2Fpub_5f45e99e6f787c06d789b64d_5f45e9eee18dd528fef43dd8%2Fscale_1200&payload=%7B+%22name%22%3A+%22social_sharing%22+%7D&skill_id=47718ad7-ee3e-4e05-94ec-dbbd8e5c5cf7&subtitle_text=Оолег+Дулин&title_text=Я+Олег+Дулин&signature=KJd5Y7b2Fym57TwZe3UheHm4GKpQkj3YmcKBS08nlfA%3D
  
запрос request.type = AudioPlayer.PlaybackNearlyFinished приводит к попеременной смене 2х треков со смещением к концу трека  
