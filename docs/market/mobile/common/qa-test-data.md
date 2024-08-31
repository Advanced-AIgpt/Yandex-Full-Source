# Тестовые данные

## 1. Тестовые девайсы
* Можно пойти в [Гиперкуб](https://wiki.yandex-team.ru/market/beru/Гиперкубы-Маркета/)
* Можно воспользоваться эмулятором. (Xcode, Android Studio)
* [Колхоз](https://kolhoz.yandex-team.ru/promo)
    * [Если нужны права на Колхоз](https://idm.yandex-team.ru/system/kolhoz/roles#role=26079838,f-role-id=26079838,f-status=all,f-role=kolhoz,sort-by=-updated)
    * [FAQ на Колхоз](https://wiki.yandex-team.ru/assessment/kolhoz/kolxoz-faq/#iii.dljavsexpolzovatelejj)
* В крайнем случае, можно пойти к тестировщику с девайсом :)

## 2. [Тестовые карты для оплаты](https://wiki.yandex-team.ru/users/yomelnikov/testing-cards/)

## 3. Тестовые учётки
* Могут быть:
    * продовые - привязанная к Стаффу/типа yndx-ChuckNorris/любого иного вида
        * [оформлять тут](https://passport.yandex.ru/profile)
    * тестовые - типа yndx-TestChuckNorris/любого иного вида
        * [оформлять тут](https://passport-test.yandex.ru/passport)
* Чтобы продовую учетку не блочили при частой отменене заказов, её следует добавить в [Антифрод](https://wiki.yandex-team.ru/market/development/antifrod-marketa/Antifraud-Support-Forms/)

* [крайне полезный клубик про создание тестовых учеток в Этушке](https://clubs.at.yandex-team.ru/passport/3360)
* [бот, где в том числе можно завести учетку](https://t.me/qabmo_bot)

{% note warning %}

Тестирование на проде конвертируется в работу с реальными заказами. Поэтому если в рамках тестирования вы оформили заказ в проде да ещё и на рандомный адрес, его нужно обязательно отменить! Это можно сделать через вкладку `Мои заказы`, в разделе `Подробднее`

{% endnote %}

## 4. Полезности
* [Как подвинуть статус товара](https://wiki.yandex-team.ru/users/irgendwer/dvizh-tovarov-po-statusam-na-teste/)
* [Как напечатать чек в тестинге](https://wiki.yandex-team.ru/users/poluektov/pechat-dostavochnyx-chekov-v-testinge/)
* [Как создать скидку](https://wiki.yandex-team.ru/users/juice/sozdat-skidku-cherez-cenoobrazovanie/)
* [Как завести БеруБонус (акцию, монетку)](https://wiki.yandex-team.ru/users/makbet/kak-zavesti-akciju-v-adminke-lojalti/)
* [Как создать акцию](https://wiki.yandex-team.ru/users/juice/kak-sozdat-akciju/)
* [Как создать промокод](https://wiki.yandex-team.ru/users/juice/kak-sozdat-promokod-v-testinge/)
* [Как создать разобранный товар](https://wiki.yandex-team.ru/users/arsentiev/bluetestingrazobralytovar/)
* [Как отправить пуш](https://wiki.yandex-team.ru/users/manvelova/Otpravka-pushejj-dlja-MP/)
* [Диплинки](https://wiki.yandex-team.ru/market/mobile/marketapps/blue-deeplinks/?from=%2FMarket%2Fmobile%2Fmarketapps%2FDezhurstva%2FBlue-Deeplinks%2F)
* [Как получить продовую КМ в тестинге](https://wiki.yandex-team.ru/users/arsentiev/testirovanie-km-v-testinge/)
* [Как прверить, должен ли отображаться CMS виджет](https://wiki.yandex-team.ru/users/arsentiev/kak-proverit-dolzhen-li-otobrazhatsja-cms-vidzhet/)
* [Как заводить сториз](https://wiki.yandex-team.ru/users/yvgrishin/instrukcija-kak-zavodit-storiz-na-sinem/)
* [Актуальные синие промо](https://wiki.yandex-team.ru/users/lisenque/production-market-promos/from=%2Fusers%2Flisenque%2Fproduction-blue-promos%2F)
* Всё о типах офферов (1Р\3Р\ДСБС)
    * [DSBS для тестирования](https://wiki.yandex-team.ru/users/fzhiyenbayeva/dsbs-v-prode-dlja-testirovanija/)
    * [Модели Маркплейса](https://wiki.yandex-team.ru/users/sophiegr/kontur-assortiment/)
    * [DSBS белый и синий](https://wiki.yandex-team.ru/users/lengl/dsbs-belyjj-i-sinijj/)
    * [DSBS hub](https://wiki.yandex-team.ru/users/vladilsavs/dsbshub/) 
    * [DSBS на белом](https://wiki.yandex-team.ru/users/pixel/zametki-belogo-kontura/dsbs-na-belom/)

## 5. Рубрика "Популярные вопросы":
### Как зайти из-под одного тестинг-аккаунта на веб и в приложение (чтоб с веба добавлять товары, а в приложении смотреть)?
- В приложении залогиниться под тестовым аккаунтом (как это сделать смотри выше в разделе  `Тестовые учетки`)
- На вэбе открыть [тестовый паспорт](https://passport-test.yandex.ru/passport) и залогиниться
- На вэбе в соседней вкладке пойти на [тестовую страничку](https://desktop.tst.market.yandex.ru/)
- Аккаунт должен подтянуться!
- Вы великолепны!

### Есть ли тестовый прод-аккаунт, в котором есть завершенные покупки?
- Формально - нет. Неформально - не сдерживай себя - приобрети что-нибудь на Маркете!

### Как посмотреть на КМ/оффер если есть `skuID`
- В приложении на КМ товара есть кнопка `поделиться`, через которую можно отправить ссылку себе туда, где удобно будет её посмотреть и отредактировать.
- Получив ссылку вида `https://market.yandex.ru/product/100682864793`, обращаем внимание на рандомный набор чисел в хвосте ссылки - это и есть **skuID**
- Всё, что нам нужно - подставить свой вариант параметра, перейти по новой версии ссылки и voila!

### Как посмотреть на КМ/оффер если есть `wareId`, он же `ware_md5`
- Почти то же самое происходит с **wareId**, только ссылка выглядит страшнее, например так: `[https://market.yandex.ru/offer/hFjhZEVJ38mtfcOfe1ke7w?cpc=0pr3jBM8sjtLXK1onRKE6KgrAczqwxejnX4ohLHkfnfy3Rf64ffh6L9KP-Kwpk1Ka_3RZNBcjmTXcMDucQqbnXBTKrQO5gcfNT46rHbfrNjWmRAG1EZtYxX2Nsre8TExZH2y_ZJ29p6krL32ZDxVOzGcHB7-h0DwCPx2o01eDPvdLt4YfuN9gV-Ykta2JK1W&hid=91013&hyperid=752265055&lr=213&modelid=752265055&nid=54544&rs=eJyz4uQQFWKQYFRiNAQABWYA4A%2C%2C&show-uid=16293648707398381004300001`
- Такую ссылку берём уже с вэб-версии Маркета. Для того, чтобы найти именно *оффер*, можно перейти в раздел `Все N предложений` на КМ и открыть любой понравившийся.
- Так же берём ссыль и обращаем внимание на странные буквы, похожие на название светильника из IKEA, расположенные между словом "offer" и "?" - это есть **wareId** 
- Подставляем параметр и чудесным образом сслыка теперь ведет вас на нужный оффер.

### Как посмотреть на КМ/оффер если есть `offerID + Supplier ID` или `Feed ID`
- Есть вариант с OTrace - на оффер также можно взглянуть имея **offerID + Supplier ID** (он же shop_ID) либо **FeedId**
- Выше указанные параметры подставляем [сюда](http://active.idxapi.vs.market.yandex.net:29334/v1/otrace) и получаем страницу со всей возможной инфой по данному офферу, в том числе и ссылку на него в поле "urls" 
- Выбираем любой подходящий вариант, проделываем эти нехитрые хитрости и вы неотразимы!

### Как найти DSBS-товары
- Воспользоваться дебажной кнопкой D в жуке (актуально для ios и вэб). В приложении инфа будет, если положить товар в корзину. На вэбе - тут же на КМ. Необходимый параметр так и называется DSBS (bool) - прим. на вэбе сейчас поломалось и отображается :(
- Вариант для вэба - консоль разработчика. Поискать по параметру `isDSBS=true`
- Если у магазина есть рейтинг и отзывы, то это dsbs
- ДСБС можно найти в фарме, в продуктах, в цифровых товарах

### как найти товар с предзаказом
- наверняка для предзаказа есть параметр, по которому можно в yql сделать запрос. Также на тесте есть яндекс станция. - ?
