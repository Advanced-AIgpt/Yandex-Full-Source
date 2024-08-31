# Лайфхаки

## Сборка

- Для того, чтобы файлы постоянно не индексировались и не ждать холодную сборку, можно счекаутить себе **2 проекта** и работать в разных ветках

## Трекер

- Для того, чтобы не терять и не забывать про свои задачи, можно настроить борду со своими задачами
Выглядит примерно так
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-06-23T10:44:03Z.f1a01b9.png)

{% cut "Как настроить" %}

1. Заходим в [стартрек](https://st.yandex-team.ru/)
2. Выбираем `Доски` -> `Создать доску`
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-06-23T10:46:45Z.aecfb5c.png)
3. Выбираем тип доски `Простая`
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-06-23T10:48:28Z.ab4ea10.png)
4. Выбираем как задачи будут попадать на доску - `автоматически` и вводим название доски
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-06-23T10:49:47Z.546bc1d.png)
5. Жмём `Добавить условие`, вводим `Исполнитель`, выбираем поле из списка и жмём `Сохранить`
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-06-23T10:52:15Z.9c32259.png)
6. Нажимаем на `Я` и выбирается исполнитель для фильтра. Жмём на продолжить
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-06-23T10:53:50Z.d0905a7.png)
7. Вводим очередь `BLUEMARKETAPPS` и выбираем из списка очередь. Нажимаем `Создать`
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-06-23T10:55:01Z.7d46357.png)
8. Доску лучше поместить в избранные, ну и вкладку с доской закрепить не помешает
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-06-23T10:56:32Z.a61fb7e.png)

{% endcut %}

## Календарь

- Чтобы видеть календарь на телефоне и часах, и не попадать под ограничения [BYOD-политик](https://doc.yandex-team.ru/calendar/common/sync/sync-mobile.html?lang=rc2), можно воспользоваться [CalDAV](https://clubs.at.yandex-team.ru/security/14785)

- Чтобы не пропускать и не опаздывать на встречи, можно настроить уведомления на них

{% cut "Как настроить" %}
1. В календаре, напротив календаря со своим именем нажмите на шестерёнку
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-06-23T11:58:58Z.140c0be.png)
2. В настройках добавьте способы и время, как и когда хотите получать уведомления и нажмите `Сохранить`
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-06-23T12:01:10Z.ea678cb.png)
3. В шестерёнке в правой части экрана можно настроить номер телефона и время, когда не беспокоить
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-06-23T12:03:05Z.182b84d.png)
4. Теперь при создании новой встречи у вас будут стоять уведомления автоматически
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-06-23T12:04:03Z.2e18b52.png)

{% endcut %}
    

## Почта

- Как написать письмо куче народу? Например всей службе. Когда я пишу письмо для сбора на ДР, то делаю так

{% cut "Рассказывай" %}

1. Открываю на стаффе группу-службу
2. Кликаю на всех сотрудников
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-06-23T13:55:46Z.aaa8542.png)
3. Нажимаю на `Написать всем`
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-06-23T13:57:43Z.ca10493.png)

Если нужно написать людям из разных групп, то:
1. Создаю письмо без получателей
2. Вышеуказаннным способом получаю список почт
3. Копирую их из сгенерированного письма и вставляю в своё
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-06-23T14:02:21Z.0dca354.png)

{% endcut %}

## Консоль

- Как ускорить работу с консолью и не вбивать по 1000 раз один и тот же набор команд?

{% cut "Нужно всего лишь..." %}

Приведу пример работы с zsh, для других консолей возможно будут немного другие правила. Есть в корневой папке пользователя такие фалый как `.bash_profile`, `.zshrc`. Можно добавить в эти файлы функции или алиасы. 
`Алиасы` - по сути это сокращения, я ими описываю очень короткие команды. Например, в моем `.bash_profile` можно увидеть такие алиасы:
```bash
alias adb='/Users/username/Library/Android/sdk/platform-tools/adb'

alias input='adb shell input text'

alias open='adb shell am start -a android.intent.action.VIEW'

alias adbr='adb reverse tcp:8888 tcp:8888'

alias detekt='./gradlew detekt'

alias inquisitor='./gradlew callInquisitor'
```
Как видишь, чтобы вызвать инквизитора или запустить detekt мне достаточно прописать в консоли команды `inquisitor` и `detekt` соответственно.

> ### Примеры использования
> ```bash
> input "password" // На устройстве в поле ввода напишет слово 'password'
>
> open https://market.yandex.ru/sku1234 // откроет диплинк в маркете
> ```

Функции я использую для более сложных ситуаций. Например, изредка мне скидывают задачи на ContentApi, и чтобы обновить локальное ContentApi до текущего состояния приходится выполнять много команд. Чтобы не делать много ctrl-c + ctrc-v, я написал простую функцию:

```bash
ucapi() {
	export CUR_DIR=$PWD
	cd ~/arc/arcadia/market/content-api/content-api/make/collect-local-start/ 
	ya make
	
	cd ~/arc/arcadia/market/content-api/content-api/make/download-test-cache/
	ya make


	cd ~/arc/arcadia/market/content-api
	mkdir -p ~/idea/content-api
	PATH=$PATH:~/arc/arcadia 
	ya ide idea --iml-in-project-root --local --project-root ~/idea/content-api -DJDK_VERSION=8

	cd $CUR_DIR
}
```
Как видишь, эта функция даже возвращается в директорию, из которой была вызвана, так что можно просто вызвать в консоли `ucapi` и по завершению команды продолжить работать в нужной тебе директории.

Стоит быть немного осторожным с редактированием `.bash_profile`, `.zshrc`, ибо есть возможность что-нибудь сломать. Чтобы проверить что вы ничего не сломали, можно перезапустить консоль (твои изменения тут же применятся к ней) либо в самой консоли вызвать `source редактируемый_файл` (замени `редактируемый_файл` на файл с правками). Это позволит не перезагружая консоль применить твои правки. А чтобы было еще проще, можешь написать такой алиас:

```bash
alias update='source ~/.bash_profile'
```

Теперь на каждую правку достаточно вызывать в консоли `update`. 

Алиасы и функции будут работать так же и в консоли AndroidStudio. Если вы запустили студию после того как закончили работу над алиасом/функцией - заработает из коробки, иначе - можно вызвать `source`, который все починит

Если вы пользуетесь линуксом, вполне возможно что у вас по умолчанию идет другая консоль (bash, или что-то еще), думаю можно либо поставить тот же zsh, либо погуглить как этот же функционал реализовать на вашей консоли (подозреваю что большинство консолей это поддерживают)

{% endcut %}

## Подстановка и изменение отдельных параметров через Proxyman

Используя инструмент Scripting в [Proxyman](https://proxyman.io/) можно автоматизировать имитацию различных результатов запроса. Например, выдавать ошибку или изменять или подставлять в результат запроса нужные параметры, которые еще не реализовали на бэкенде.

Удобно, потому что:
* Работаешь со структурой ответа (json), а не с заменой текста через регулярки (как в Charles).
* Можно изменять отдельные параметры или подставлять новые в разные запросы, а не заменять ответ целиком.
* **Эти скрипты удобно отдавать тестировщикам** для проверки разных кейсов, если бэкенд еще не готов или сложно настраивается, или нужный кейс сложно найти.

[Документация на сайте Proxyman](https://docs.proxyman.io/scripting/script)

{% cut "Чтобы воспользоваться нужно сначала установить и настроить Proxyman (бесплатно)" %}

* скачать Proxyman
* установить сертификат на мак: Certificate → Install Certificate on this Mac… → следовать указаниям
* для настройки iOS/Android-симулятора: меню Certificate → Install Сertificate on iOS/Android → Simulators → следуем указаниям
* для использования на устройстве: 
  * меню Certificate → Install Сertificate on iOS/Android → Physical Devices → следуем указаниям
  * на устройсте подключиться к тому же вайфаю, что и мак, в настройках вайфая настроить прокси на ip и порт из заголовка окна Proxyman

{% endcut %}

##### Настроить выполнение скрипта:
* меню Scripting → Scrit list → +
* заполняем название, маску URL запроса, который хотим менять и что хотим менять — request и/или response
* в подставленном коде реализуем функцию onResponse или onRequest (в зависимости от того, что перехватываем и хотим изменить)

> Могут быть накладки при выполнении скриптов из-за одинаковых URL-паттернов, тогда важен порядок запуска. Тогда более точный нужно ставить выше в очереди, например, сначала \*resolveSearchIncut\*, потом \*resolveSearch\* 

> Выполнение скриптов не работает одновременно с перехватом запросов через Tools → Breakpoints

##### Примеры скриптов

{% cut "Добавить searchConfiguration в результат поиска (настройка отображения выдачи)" %}

URL: `*resolveSearch*`

```javascript
function onResponse(context, url, request, response) {
  var body = response.body;
  var collections = body["collections"];
  if (collections != undefined) {
    collections["searchConfiguration"] = [{
                "template": {
                    "blocks":{
                        "actions": {
                            "showCompareButton": true,
                            "showWishlistButton": false,
                            "type": "actions",
                            "visible": true,
                        },
                        "disclaimer": {
                            "type": "disclaimer",
                            "visible": true,
                        },
                        "offer": {
                            "type": "offer",
                            "visible": false,
                        },
                        "photo": {
                            "type": "photo",
                            "visible": false,
                        },
                    },
                    "type": "detailed"
                }
          }];
   collections["visibleSearchResult"][0]["resultsViewType"] = "list"
  }
  response.body = body;
  return response;
}
```

{% endcut %}

{% cut "Ограничить количество офферов в запросе спонсорских товаров" %}

URL: `*resolveSearchIncut*`

```javascript
// ограничиваем количество спонсорских офферов
function onResponse(context, url, request, response) {
  
  const offersToShow = 2
  
  var body = response.body;
  var offers = body["collections"]["offer"];
  if (offers != undefined) {
    body["collections"]["offer"] = offers.slice(1,offersToShow+1)
  }
  response.body = body;
  return response;
}
```

{% endcut %}

{% cut "Случайная ошибка поиска (можно использовать для других запросов поменяв резолвер)" %}

URL: `*resolveSearch*`

```javascript
function getRandomInt(min, max)
{
  return Math.floor(Math.random() * (max - min + 1)) + min;
}

// случайно решаем будет ошибка или нет
// если будет, убираем result и добавляем error для нужного хендлера targetHandler
function onResponse(context, url, request, response) {
  const attepmtsCount = 2
  const showError = getRandomInt(1,attepmtsCount) == 1
  if (!showError) {
    return response;
  }
  
  const targetHandler = "resolveSearch"
  var body = response.body;
  var results = body["results"];
  if (results != undefined) {
    results.forEach( result => {
      if (result["handler"] == targetHandler) {
        results[0]["result"] = null
        results[0]["error"] = "unknown"
      }
    });
  }
  response.body = body;
  return response;
}
```

{% endcut %}

{% cut "Непроставленная оценка у заказа (запрос обратной связи на морде и в заказах)" %}

URL: `*resolveOrderGrades*`

В скрипте нужно заменить orderId на свой.

```javascript
var orderId = 32935264

function onResponse(context, url, request, response) {
  var body = response.body;
  var collections = body["collections"];
  if (collections != undefined) {
    collections["orderFeedback"] = [{
      "id": orderId,
      "grade": 0,
      "isReviewDenied": false,
      "isReviewSubmitted": false
    }];
  }
  var results = body["results"];
  if (results != undefined) {
    results.forEach( result => {
      if (result["handler"] == "resolveOrderGrades") {
        result["result"] = [orderId]
      }
    });
  }
  response.body = body;
  return response;
}
```

{% endcut %}

{% cut "Получение эксперимента с бэка" %}

URL: `*startup*`

Делаем вид, что получаем эксп с бэкенда. Может пригодиться, если нужно, например, проверить отключение реарра тогглом.
В скрипте нужно заменить параметры эксперимента на нужные.

```javascript
var rearr = "market_metadoc_search=skus"
var alias = "sku_search"

function onResponse(context, url, request, response) {
  var body = response.body;
  var experiments = body["experiments"];
  if (experiments != undefined) {
    experiments.push({
      "testId": "999999",
      "bucketId": "777",
      "alias": alias,
      "backendExp": true,
      "rearrFactors": [rearr]
    });
  }
  response.body = body;
  return response;
}
```

{% endcut %}

## Загрузка картинок на статик сервер
Может понадобиться, если нужно хранить картинку (или любой другой ресурс) на сервере. Самый простой способ - загрузить на статик сервер.

Ссылка на репозиторий статики - https://github.yandex-team.ru/market/export-static

Как пользоваться: добавить картинку в репозиторий, затем по инструкции в ReadMe опубликовать новую версию в stable.
