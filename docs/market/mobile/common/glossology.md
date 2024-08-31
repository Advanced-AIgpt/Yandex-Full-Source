## Глоссарий

### Б

#### БПВЗ
Брендированные пункты выдачи заказов «Яндекс.Маркет»

### В

#### Виджет
Некоторая самостоятельная сущность на странице. Умеет получить все необходимые для отрисовки данные, опираясь на переданные параметры

#### Виртуальные SKU
Оффера, для которых изначально не была найдена модель и SKU создается виртуальная SKU. Причем modelId будет равен skuId. Но так не всегда, существуют оффера без модели и SKU. [Статья](https://wiki.yandex-team.ru/users/juice/modelstructure/) [Доклад](https://wiki.yandex-team.ru/users/dmpolyakov/poleznosti-s-ezhenedelnojj-vstrechi/#proskuidmodelidofferidvprilozhenijaxdljavsex)

### Д

#### Дежурство
Период в работе, когда с сотрудника снимаются обязанности и задачи контура и приходится заниматься другими задачами Маркета, в зависимости от вида дежурства. [Подробнее](https://docs.yandex-team.ru/market-mobile/common/duty/common)

#### ДО
Дефолтный оффер — дефолтное товарное предложение. Репорт благодаря хитрому алгоритму признаёт один из офферов на каждой [КМ](https://docs.yandex-team.ru/market-mobile/common/glossology#КМ) как «дефолтный» — с максимальными шансами ублажить пользователя (или Маркет) по ряду параметров. Цену именно этого оффера пользователь наблюдает на самом видном месте

### К

#### Карусель
Блок-слайдер отображающий несколько сниппетов с возможностью скролла вправо/влево

#### КМ / Модель
Карточка модели или модель — логически собранная группа товаров без комплектации (цвет, размер и прочее). Например "Apple iPhone XR". [Статья](https://wiki.yandex-team.ru/users/juice/modelstructure/) [Доклад](https://wiki.yandex-team.ru/users/dmpolyakov/poleznosti-s-ezhenedelnojj-vstrechi/#proskuidmodelidofferidvprilozhenijaxdljavsex)

#### КО / Оффер (offer)
Карточка оффера или оффер — товарное предложение от определенного магазина, а именно совокупность всех предлагаемых продавцом характеристик: цена, описание товара, методы оплаты, сроки доставки и другое. Например
"Apple iPhone XR 64 Гб, коралл от Яндекс.Маркета за 45 000р". Именно оффера и покупают пользователи. [Статья](https://wiki.yandex-team.ru/users/juice/modelstructure/) [Доклад](https://wiki.yandex-team.ru/users/dmpolyakov/poleznosti-s-ezhenedelnojj-vstrechi/#proskuidmodelidofferidvprilozhenijaxdljavsex)

#### Контур
Команда с определенной сферой обязанностей, например Контур#Фарма — это все что связано с аптечными товарами. [Подробнее](https://wiki.yandex-team.ru/users/ilya-khudy/blue-market/testing/vteams/)

### М

#### Морда
Традиционное название главной страницы сервиса в Яндексе

### П

#### ПВЗ
Пункт выдачи заказов

### Р

#### Репорт
Бекенд Маркета, отвечает за поиск в Маркете и информацию об офферах и моделях. [Подробнее](https://wiki.yandex-team.ru/Market/Verstka/report-query-params/)

#### Ручка (handler)
Название http-endpoint'а в Яндекс.Маркете

#### РСЯ 
Рекламная сеть Яндекса

### С

#### СиС 
Срок и стоимость доставки на Маркете. [Подробнее](https://wiki.yandex-team.ru/market/projects/multiregion/)

#### Сниппет 
Ссылочный блок, кратко представляющий отдельный результат поиска: с картинкой и описанием

#### Сплит 
Один из вариантов эксперимента. [Подробнее](https://wiki.yandex-team.ru/Market/Verstka/abt2/)

### Т

#### [Тач](https://m.market.yandex.ru/)
Мобильная версия Маркета в браузере

#### Топовые офферы 
Список офферов, которые хитрый алгоритм в Репорте считает самыми "лучшими"

#### [Трассировка](https://tsum.yandex-team.ru/trace)
Возможность по id запроса пользователя `X-Market-Request-Id` отследить весь путь взаимодействия с компонентами Маркета, от параметров запроса к фронту, до информации о запросах в бекенды, времени ответа и т.д. Можно посмотреть весь путь, если скопировать `X-Market-Request-Id` из ответа запроса и вставить значение в ссылку `https://tsum.yandex-team.ru/trace/{X-Market-Request-Id}`. Откроется сайт со списком запросов, где видно, какие запросы совершались, какие прошли, а какие - нет.

### Ц

#### [ЦУМ](https://tsum.yandex-team.ru)
Сервис, позволяет понять что происходит с Маркетом, выполнить трассировку запроса, а так же посмотреть таймлайн событий

### C

#### [CMS](https://cms.market.yandex.ru)
Content Management System — система управления контентом, позволяет описать контент в виде страниц и отрисовывать его на фронтах. [Подробнее](https://docs.yandex-team.ru/market-mobile/common/cms)

### D 

#### Deeplink
Путь до какой то части приложения. Парсится при открытии. Таким образом можно: перейти в личный кабинет, открыть какой то товар, список избранных и т.д. [Подробнее](
https://wiki.yandex-team.ru/market/mobile/marketapps/blue-deeplinks/?from=%2FMarket%2Fmobile%2Fmarketapps%2FDezhurstva%2FBlue-Deeplinks%2F)

### F

#### FAPI (ФАПИ)
Front API — фасадный бекенд, предоставляет доступ мобильным приложениям к [резолверам](https://docs.yandex-team.ru/market-mobile/common/glossology#resolver) синего фронта. В качестве публичного api предоставляется единственная [ручка](https://docs.yandex-team.ru/market-mobile/common/glossology#ruchka-handler) формата `POST https://<host>/api/<version>/` . Целевое действие и ответ ручки определяется набором резолверов. Резолверы перечисляются в query параметре name через запятую

#### FBS
Fulfillment by Seller — модель работы, в рамках которой поставщик сам хранит товары на своем складе, принимает заказы от маркетплейса и упаковывает их. Маркетплейс представляет товары на витрине, общается с покупателями и доставляет заказы

#### FBY 
Fulfillment by Yandex — модель работы, в рамках которой поставщик поставляет товары на склады маркетплейса. Маркетплейс хранит их и представляет на витрине, обрабатывает, собирает и упаковывает заказы, отвечает за доставку и общается с покупателями

#### Feature Toggle
Механизм, позволяет включать или отключать фичи или логику мобильного приложения, без обновления версии приложения и выкладывания в сторы релиза. [Подробнее](https://docs.yandex-team.ru/market-mobile/common/remote-configuration)

### H

#### Hid 
Идентификатор категории. [Подробнее](https://wiki.yandex-team.ru/hidornid/)

### M

#### MBO
Market Back Office — набор сервисов, отвечающий за обработку информации о товарах и предложениях

### N

#### Nid 
Идентификатор узла навигационного дерева. [Подробнее](https://wiki.yandex-team.ru/hidornid/)

### R

#### Rearr factors / Rearr флаги
Похож на [Feature Toggle](https://docs.yandex-team.ru/market-mobile/common/glossology#feature-toggle), но только для бэкенда. [Подробнее](https://docs.yandex-team.ru/market-mobile/common/rearr-factors)

#### Resolver
Резолвер — cерверный метод. Может возвращать статические данные, вычислять ответ по входным параметрам или возвращать данные с других бекендов. Так же резолвер может выполнять целевое действие, например: добавлять товар в корзину, подписывать пользователя на email рассылку и тд. [Список резолверов](https://pages.github.yandex-team.ru/market/marketfront/fapi/index.html)

#### RPS 
Requests per second - количество запросов к компоненту в секунду. Меняется в зависимости от времени суток, телепередач, выходных и праздников и даже погоды

### S

#### SKU
Stock Keeping Unit — складская единица учета, характеризующая конкретный товар. Например "Apple iPhone XR 64 Гб, коралл". [Статья](https://wiki.yandex-team.ru/users/juice/modelstructure/) [Доклад](https://wiki.yandex-team.ru/users/dmpolyakov/poleznosti-s-ezhenedelnojj-vstrechi/#proskuidmodelidofferidvprilozhenijaxdljavsex)

#### SSKU 
shopSku — идентификатор оффера магазином, используется при создании акций. [Подробнее](https://wiki.yandex-team.ru/users/juice/modelstructure/)

#### [Staff (Стафф)](https://staff.yandex-team.ru)
Страница в Яндексе с информацией о сотрудниках

### V

#### VSID
Video Session ID — идентификатор сессии в плеере и STRM. [Подробнее](https://wiki.yandex-team.ru/player/vsid/)

### W

#### wareMd5
Тоже что и offerId. [Подробнее](https://wiki.yandex-team.ru/users/juice/modelstructure/)

### Y

#### [YQL](https://yql.yandex-team.ru)
Язык запросов к БД, сделаный в яндексе, похож на SQL, но со своими плюшками. [Подробнее](https://yql.yandex-team.ru/docs/)
