# Пример расследования инциндента от 05.09.2021

## Общее описание проблемы

Утром 05.09.2021, в догфудинге участились жалобы на неработающую морду в android приложении.

![alt text](https://jing.yandex-team.ru/files/kaktus23/duty_investigation_1.png)

@[youtube](https://www.youtube.com/watch?v=-zywFdFyNu4)

На видео видно, что пользователь пытается несколько раз перезагрузить морду, но это не помогает.
Так же другой пользователь сообщил, что у него проблема сохраняется в течении 15 минут.

Вместе со скрином проблемы, была прислана [трассировка](https://acelost.github.io/?jsonbinio=613477d6dfe0cf16eb554f72).


## Процесс решения проблемы

Ниже описано как мы решали проблему, а так же преведен список гипотиз, которые мы проверяли, для решения проблемы.

### 1. Много одинаковых запросов в трассировке
В [трассировке последнего запроса](https://tsum.yandex-team.ru/trace/1630828496459/6e947e75722c61ea5e9538d93acb0500) из [всех запросов пользователя](https://acelost.github.io/?jsonbinio=613477d6dfe0cf16eb554f72), было много на первый взгляд одинаковых запросов из market-checkouter в pgass.

![alt text](https://jing.yandex-team.ru/files/kaktus23/duty_investigation_2.png)

Я написал дежурному по чекаутеру. Дежурный сказал, что все ок, а так же сообщил, что pgass это postgresql. Оказалось, что много запросов в postgresql от чекаутера является нормальным поведением.

### 2. Просмотр событий здоровья fapi

Далее мы отправились смотреть [события здоровья резолверов fapi, по конкретному пользователю](https://yql.yandex-team.ru/Operations/YTZ2qgVK8AsKxijB1Fkt4y9FndeK2fz-14VgCbo9Xsk=).
Пользователя из трассировки мы определяли по uuid. Его можно взять из любого запроса в трассировке.

![alt text](https://jing.yandex-team.ru/files/kaktus23/duty_investigation_3.png)

В ответе от `YQL`, в столбце `stack_trace` были только `java.net.ConnectException` или `java.io.InterruptedIOException`. `InterruptedIOException` может происходить если пользователь ушел с экрана до того, как резолвер Fapi завершил свою работу, поэтому мы рассматривали только ошибки `java.net.ConnectException`. C помошью [этого запроса в YQL](https://yql.yandex-team.ru/Operations/YTSG69K3DJRSYS0aVcOHXyJic4j68Vdz7Qy3lgxgj1I=) мы оценили количество уникальных пользователей, которые ловили `java.net.ConnectException` за последние дни.
04.09.2021 - 6076 пользователя
05.09.2021 - 1863 пользователя (на момент времени 12:05)

Сделали вывод, что это естевственный фон ошибок сети, и принялись искать дальше.

### 3. Просмотр всех событий здоровья пользователя
Удалив из запроса для событий здоровья fapi, фильтрацию по коду ошибки, мы получили [запрос выводящий все ошибки пользователя](https://yql.yandex-team.ru/Operations/YTaGWQVK8AsKxjE9y1QT7QrqdAxuh0XUCFjttZVzuqE=). После изучения результатов запроса, методом пристального взгляда, в столбце `extra_values`, было обнаружено подозрительное исключение.
```
java.lang.RuntimeException: Widgets list is empty for page CmsPageId(id\\u003droot)
```
Поиском по github я нашел место в коде, где это исключение выбрасывается.

![alt text](https://jing.yandex-team.ru/files/kaktus23/duty_investigation_4.jpg)

### 4. Статистика ошибки `Widgets list is empty` по датам

С помощью [этого запроса в YQL](https://yql.yandex-team.ru/Operations/YTaJt5fFt7V0YaolJLLmkfNpobX8gMj4gLtl4sOpbqQ=), я получил количество ошибок `Widgets list is empty`, за последние дни.
```
31.08 - 45
01.09 - 40
02.09 - 30145
03.09 - 67582
04.09 - 64544
05.09 (на 12:35) - 25795
```
02.09 произошел резкий всплеск подобных ошибок.

### 5. Морда в редакторе CMS
 Зайдя в [редактор CMS](https://cms.market.yandex.ru/editor/), и найдя в нем страницы морды приложения, я увидел несколько версий морды, которые обновляли 02.09
![alt text](https://jing.yandex-team.ru/files/kaktus23/duty_investigation_5.jpeg)

### 6. Схожие проблемы на iOS
В это же время, в дежурный чат мобилок прилетело видео с аналогичной проблемой на ios.

@[youtube](https://youtu.be/L1-KzoAUoew)

Видео сопровождалось, комментарием от пользователя.
```
iOS 14.7.1
На главную - не выходит, обновить - не работает
```
Поскольку проблемы были на обеих платформах, мы еще больше стали подозревать cms.

### 7. Детализация ошибок cms по часам

В 13:16 руководитель группы разработки рекоммендаций написал, что дата начала ошибок совпадает с запусками их экспериментов. Что бы подвердить связь, он попросил сделать детализацию ошибок 02.09
по времени. С помощью [еще одного запроса в YQL](https://yql.yandex-team.ru/Operations/YTaO8a5ODwr4mVZegEZ4Gt69-_5OzEuFx2tt3Dbp5H8=), я постоил эту детализацию.
![alt text](https://jing.yandex-team.ru/files/kaktus23/duty_investigation_6.jpg)
Резкий всплеск произошел 02.09.2021 в 16:00

Экспы рекоммендаций выехали 02.09.2021 в 18:56, по времени не совпало.

### 8. Схожая ошибка раздела Express

В 13:59, в дежурный чат мобилок прилетела схожая проблема в разделе Express.
![alt text](https://jing.yandex-team.ru/files/kaktus23/duty_investigation_7.jpeg)

Это подтверждало гипотизу о проблемах в cms, поскольку в разделе Express, тоже используется cms

### 9. Поиск кривых страниц в cms
У нас была гипотиза о том, что одну из страниц cms неудачно обновили и она сломана. Для подтверждения гипотизы, нужно было воспроизвести эту проблему у себя. Я пытался воспроизвести ее, для страниц морды, которые обновляли 02.09, при помощи следующих шагов:

1. Брал rearr-флаг, который используется для эксперимента со страницей морды, из редактора cms
![alt text](https://jing.yandex-team.ru/files/kaktus23/duty_investigation_8.png)
2. На qa сборке, внизу экрана экспериментов указывал этот флаг
3. Перезапускал приложение и видел морду из эксперимента

Ни с одним из rearr флагов, проблема у меня не воспроизвелась и морда корректно открылась.

### 10. Эксперименты у пользователей, которые столкнулись с проблемой

Далее, я попросил двух пользователей android, сообщавших о проблеме, скинуть эксперименты в которые они попали. Для этого нужно на странице "О приложении", зажать логотип (Профиль->Настройки->О приложении). После этого информация об экспериментах попадет в буфер обмена.
Пользователи прислали следующую информацию:
```
UUID: 9010ec3838014700a612ce9ba78cf224, Experiments: 0,0,
```
```
UUID: de955a4e63394ffd8e3be539d9da87f5, Experiments: 0,0,
```
Оба пользователя, которые столкнулись с проблемой, вообще не участвовали в экспериментах.

Используя [запрос в YQL](https://yql.yandex-team.ru/Operations/YTSoI9K3DJRSYVAEcofeotBrcWAgMLROLPlJvyDAHv0=) я получил информацию, об экспериментах пользователей, столкнувшихся с проблемой.

![alt text](https://jing.yandex-team.ru/files/kaktus23/duty_investigation_9.jpeg)
Большая часть пользователей, столкнувшихся с проблемой 05.09, вообще не участвовала в экспериментах.

### 11. Перевод экспериментов на fapi
01.09.2021 в 19:08 выехал [эксперимент, по переводу эксперементов с CAPI на FAPI](https://st.yandex-team.ru/EXPERIMENTS-77433). Мы стали подозревать, что переход прошел неудачно. Для начала я выставил на экране экспериментов, эксперимент `Fapi Experiments` в состояние `FAPI_EXPERIMENTS_TEST`. После чего я вновь проверил работу страниц cms, которые обновляли 02.09.

У меня они корректно открывались. Поскольку, большая часть пользователей, столкнувшихся с проблемой 05.09, вообще не участвовала в экспериментах, далее было решено проверить поведение`FAPI_EXPERIMENTS_TEST` на пустом наборе экспериментов. Набор экспериментов пользователя летит на бекенды с мобилок. Android забирает набор экспериментов при старте приложения, но забирает только в base сборке. Что бы проверить пустой набор экспериментов, я собрал локальную baseDebug сборку, в которой зашил пустой набор экспериментов. У меня морда все равно открывалась корректно.

### 12. Просмотр среднего числа ошибок на пользователя

Поскольку у нас долго не получалось воспроизвести проблему, мы стали подозревать что проблема флакает на бекендах. Что бы это подвердить, я [вычислил среднее количество ошибок на одного пользователя](https://yql.yandex-team.ru/Operations/YTTAwRJKfVebkcg8bEpeBb8w8eOl84TKk97Y0OYx4VM=). На момент 16:06, получилось `3.7226398601398603`. Маленькое число ошибок на одного пользователя подтверждало гипотизу о флакающей проблеме.

### 13. Проверка наличия схожей проблемы на iOS

Что бы подтвердить наличие аналогичной проблемы на iOS, я отправился изучать графики iOS на [нашей борде здоровья приложений](https://nda.ya.ru/t/NT_T6Tcw3W7qvs). Методом пристального взгляда был обнаружен всплеск ошибок `CMS_NOT_LOADED` 02.09.
![alt text](https://jing.yandex-team.ru/files/kaktus23/duty_investigation_10.jpeg)
Далее я нашел это событие в [коде iOS приложения](https://arcanum.yandex-team.ru/arc_vcs/mobile/market/ios/app-market/BlueMarket/Classes/Controllers/Morda/Presenter/MordaPresenter.swift), и убедился что оно стреляет по схожим причинам.
![alt text](https://jing.yandex-team.ru/files/kaktus23/duty_investigation_11.jpg)

### 14. Проблема воспроизвелась

Далее от безысходности, я стал много раз перезапускать приложение, через остановку процесса, в надежде поймать проблему. Примерно через 30 перезапусков проблема у меня все-таки воспроизвелась. Поведение приложения было у меня в точности как на первом видео в этой статье. Кнопка "попробывать еще раз" не помогала.
Я подцепился к приложению дебагером, не перезапуская его, что бы не упустить проблему(через кнопку `Attach Debugger to Android Process`). Далее с подключенным дебагером, я проследил что происходит, при нажатии на кнопку "попробывать еще раз".
![alt text](https://jing.yandex-team.ru/files/kaktus23/duty_investigation_12.jpeg)

Оказалось, что после того как страница cms загрузится из сети, она сохраняется в кеш на время жизни процесса и больше не перезапрашивается. Это объясняло почему несмотря на флакающий характер проблемы, пользователь не мог открыть морду на протяжении 15 минут. После того как я перезапустил приложение, морда успешно открылась. Это подверждало флакающий характер проблемы.

### 15. Подтверждение флакающий проблемы на cms

Для того что бы подвердить, что cms иногда возвращает пустой набор виджетов для морды я решил взять запросов на получение морды из cms и поворить его много раз пока проблема не воспроизведется. Для этого я взял запрос морды в cms из charles и скопировал его как curl. Этот curl я импортировал в postman. А из postman я экспортировал запрос для python библиотеки requests.
![alt text](https://jing.yandex-team.ru/files/kaktus23/duty_investigation_13.png)

Для того что бы делать запросы c помощью `requests`, `requests` нужно установить.
```
pip3 install requests
```

Далее я написал простой скрипт, который каждые 3 секунды отправлял этот запрос, и получив в ответ пустой набор виджетов останавливался, выводя при этом тело ответа, и заголовки ответа. Заголовки нужны потому что в них лежит трассировка.

```python
import os
from time import sleep

import requests
import json

import requests



def doReq():
	url = "https://ipa.market.yandex.ru/api/v1/?name=resolveCms&uuid=7643be907f46409aad402c0f1addbc9f&rearr-factors=;enable_express_cpa_benefit=1;express_offers_hyperlocality=1;market_cashback_for_not_ya_plus=1;market_cpa_only_enabled=0;market_promo_blue_cheapest_as_gift_4=1;market_promo_blue_flash=1;market_promo_blue_generic_bundle=1;market_promo_spread_discount_count=0;market_rebranded=1;market_white_cpa_on_blue=2;show_credits_on_white=1;sku_offers_show_all_alternative_offers=1&gps=37.622504,55.753215"

	payload = json.dumps({
	  "params": [
	    {
	      "type": "mp_morda_app",
	      "app_property": "appsWithNewSingleAction",
	      "one_of": "app_property,"
	    }
	  ]
	})
	headers = {
	  'Content-Type': 'application/json',
	  'api-platform': 'ANDROID',
	  'X-Test-Id': '0,0,',
	  'X-App-Version': '3.38',
	  'User-Agent': 'Beru/3.38 (Android/11; Pixel 4a (5G)/google)',
	  'X-Market-Rearrfactors': ';enable_express_cpa_benefit=1;express_offers_hyperlocality=1;market_cashback_for_not_ya_plus=1;market_cpa_only_enabled=0;market_promo_blue_cheapest_as_gift_4=1;market_promo_blue_flash=1;market_promo_blue_generic_bundle=1;market_promo_spread_discount_count=0;market_rebranded=1;market_white_cpa_on_blue=2;show_credits_on_white=1;sku_offers_show_all_alternative_offers=1',
	  'X-Region-Id': '213',
	  'X-User-Authorization': 'ТУТ БЫЛ МОЙ ТОКЕН',
	  'Host': 'ipa.market.yandex.ru'
	}

	response = requests.request("POST", url, headers=headers, data=payload)
	return response


while True:
	resp = doReq()
	l = len(resp.text)
	print(l)
	#print(resp.text)
	if (l < 25000):
		print(resp.headers)
		print(resp.text)
		break
	sleep(3) #sleep нужен, что бы не положить бекенды высокой нагрузкой и не попасть в блокировку из-за большого числа запросов
```

Скрипт довольно быстро остановился, выведя следующее:
``` json
{'Strict-Transport-Security': 'max-age=31536000', 'Transfer-Encoding': 'chunked', 'content-encoding': 'gzip', 'content-type': 'application/json; charset=utf-8', 'date': 'Sun, 05 Sep 2021 14:37:03 GMT', 'device_type': 'market_front_blue_api', 'set-cookie': 'yandexuid=7951600381630852623; Domain=.ipa.market.yandex.ru; Path=/; Expires=Fri, 05 Sep 2031 14:37:03 GMT; Secure, skid=7984197731630852623; Domain=.ipa.market.yandex.ru; Path=/; Expires=Fri, 05 Sep 2031 14:37:03 GMT; Secure, _yasc=fDviIvxLWVSBKshegin5qkH3O2E1nwLPRypTevz2bSZL4qOs; domain=.yandex.ru; path=/; expires=Tue, 05-Oct-2021 14:37:03 GMT; secure, i=zOgyZltLeKOY2Fdlp4RrYfPS7Yt510pv3mqIkqbWns9+9VEcwdJ7C1lnx2d/T79McE5VWxcAuMpMMKBoGB6+lAYlR1c=; Expires=Tue, 05-Sep-2023 14:37:03 GMT; Domain=.yandex.ru; Path=/; Secure; HttpOnly', 'x-market-req-id': '1630852623175/e64da24569ed04813411497740cb0500', 'x-page-id': 'fapi:api', 'x-page-type': 'node', 'x-passportuid': '484250805', 'x-powered-by': 'Stout'}
{"results":[{"handler":"resolveCms","result":[]}],"collections":{"cmsDeclaration":[],"cmsEntrypoint":[]}}
```
[Трассировка этого запроса](https://tsum.yandex-team.ru/trace/1630852623175/e64da24569ed04813411497740cb0500).

### 16. Локализация проблемы

Далее нужно было определить сервис из трассировки, на котором происходит проблема. Под подозрением давно был темплатор. Я взял запрос к нему из [трассировки](https://tsum.yandex-team.ru/trace/1630852623175/e64da24569ed04813411497740cb0500). И повторяя шаги описанные выше, обновил свой скипт, так что бы он ходил напрямую в темплатор.

``` python
import os
from time import sleep

import requests
import json

import requests


def doReq2():
	url = "http://templator.vs.market.yandex.net:29338/tarantino/getcontextpage?format=json&zoom=full&device=phone&domain=ru&rearr-factors=%3Benable_express_cpa_benefit%3D1%3Bexpress_offers_hyperlocality%3D1%3Bmarket_cashback_for_not_ya_plus%3D1%3Bmarket_cpa_only_enabled%3D0%3Bmarket_promo_blue_cheapest_as_gift_4%3D1%3Bmarket_promo_blue_flash%3D1%3Bmarket_promo_blue_generic_bundle%3D1%3Bmarket_promo_spread_discount_count%3D0%3Bmarket_rebranded%3D1%3Bmarket_white_cpa_on_blue%3D2%3Bshow_credits_on_white%3D1%3Bsku_offers_show_all_alternative_offers%3D1&type=mp_morda_app&app_property=appsWithNewSingleAction&one_of=app_property%2C&puid=484250805&uuid=7643be907f46409aad402c0f1addbc9f&yandexuid=7951600381630852623&client=android&ignore_cgi_params=puid%2Cyandexuid%2Cuuid%2Cdeviceid%2Cidfa%2Cgaid%2Cclient%2Ccart&region=213"

	payload={}
	headers = {}

	response = requests.request("GET", url, headers=headers, data=payload)
	return response


while True:
	resp = doReq2()
	l = len(resp.text)
	print(l)
	#print(resp.text)
	if (l < 25000):
		print(resp.headers)
		print(resp.text)
		break
	sleep(3)
```

Результат работы обновленного скрипта не заставил себя ждать.
```
267
{'Server': 'nginx', 'Date': 'Sun, 05 Sep 2021 14:43:25 GMT', 'Content-Type': 'application/json; charset=utf-8', 'Transfer-Encoding': 'chunked', 'Connection': 'close', 'content-encoding': 'gzip', 'x-market-req-id': '1630853005132/5b80cd057e1482cf07b7e310fa13b667'}
{
"__info__": {
  "version":"2021.08.31",
  "hostname":"iva1-4155-5a8-iva-market-prod--91a-16123.gencfg-c.yandex.net",
  "collection":"cms-context-relations",
  "caller":"GetContextPageSaas",
  "servant":"templator",
  "uptime":"163538.817986s"
},
"result": [
]
}
```

Далее мы добавили в чат проблемы дежурного по темплатору. Оказалось что на темплаторе с 16:00 02.09 наблюдался повышеный фон ошибок доступа к данным.

![alt text](https://jing.yandex-team.ru/files/kaktus23/duty_investigation_14.png)

### 17. Финал

Таким образом к 18:00 проблема была локализована, а к 21:45 устранена и [график ошибок cms](https://yql.yandex-team.ru/Operations/YTUTtgVK8AsKxY60v2ob0cFQ6TkjhDoGLRlCbmR5Fhs=) резко пошел вниз.
![alt text](https://jing.yandex-team.ru/files/kaktus23/duty_investigation_15.jpeg). Оказлось, что из-за учений во Владимире 02.09, на некоторых инстансах хранилищ документов, потерялся документ с мордой приложения.

