### Шина событий:

Работа происходит по принципу publish-subscribe.

Шина реализована синглтоном, доступ до которого напрямую понадобиться не должен. Но если уж совсем захочется, то внутри есть функция `getEventBus()`.

#### 1. Объявление нового типа событий
```js
import { EventType } from '~core/event-bus';
...
const eventTypeString = new EventType<string>('string event type');
const eventTypeVoid = new EventType('void event type');
```

#### 2. Подписка на события
* Для функциональных компонентов
```js
import { useEventBusListener } from '~core/event-bus';
...
useEventBusListener(eventTypeString, payload => { console.log(payload); });
useEventBusListener(eventTypeVoid, () => { console.log('no payload'); } );
```
* Для классовых компонентов
```js
import { EventBusSubscription } from '~core/event-bus';
...
<>
  <EventBusSubscription eventType={eventTypeString} listener={console.log} />
  <EventBusSubscription eventType={eventTypeVoid} listener={console.log} />
</>
```

#### 3. Публикация событий
```js
import { publishToEventBus } from '~core/event-bus';
...
publishToEventBus(eventTypeString.create('LocoRoco'));
publishToEventBus(eventTypeVoid.create());
```
