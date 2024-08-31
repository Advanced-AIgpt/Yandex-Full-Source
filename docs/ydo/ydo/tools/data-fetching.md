### Data Fetching

Является адаптером [react-query](https://react-query.tanstack.com/overview) для Услуг, решающим вопрос Server Side Rendering-а.
На клиенте использует react-query, а на сервере подменяет своей реализацией, которая прогоняет адаптеры.

#### 1. Объявление API для похода за данными на клиенте

```typescript
export interface ISomeDataParams {
    id: string;
}

export interface ISomeData {
    // декларация загруженных данных
}

export const fetchSomeData = (params: ISomeDataParams): Promise<ISomeData> => {
    return baseInternalRequest<ISomeData>({
        endpointName: 'fetchSomeData',
        // data fetching не создаёт автоматически обработчик для этого урла
        // его работоспособность нужно обеспечить разработчику
        url: 'get_some_data',
        method: 'POST',
        data: {
            data: { params },
        },
    });
};
```

#### 2. Объявление источника данных

```typescript
export const someDataSource = dataSource<ISomeDataParams, ISomeData>({
    name: 'someData',
    fetch: fetchSomeData,
});
```

#### 3. Объявление адаптера для источника данных

```typescript
@ydo.injectable()
export class SomeDataAdapter extends Adapter {
    // не обязательно под каждый endpoint создавать отдельный класс адаптера
    // если уже есть подходящий по смыслу - лучше добавить функцию с аннотацией адаптера в него
    @dataSourceAdapter(someDataSource)
    mapSomeData(data: ApphostData, params: ISomeDataParams): ISomeData {
        if (noServerSideRenderingNeeded) {
            throw new NoServerSideData();
        }

        return {};
    }
}
```

Если создавали новый класс для адаптера - не забываем добавлять его в [IAdapters](https://a.yandex-team.ru/arcadia/frontend/services/ydo/src/adapters/IAdapters.ts) и [adaptersFactory](https://a.yandex-team.ru/arcadia/frontend/services/ydo/src/adapters/adaptersFactory.ts). Название ключа не имеет значения.

#### 4. Использование в контейнере

Для похода за данными нужно знание о жизненном цикле компонента (чтобы переставать обновлять данные после unmount-а компонента). Поэтому использовать data-fetching в react-redux connector-ах невозможно. Чтобы обойти это ограничение - нужно использовать функциональный компонент в качестве контейнера и использовать в нём `useDispatch` и `useSelector`.

```typescript
interface ISomeComponentContainerProps {
    id?: string;
}

const SomeComponentContainerPresenter: React.FC<ISomeComponentContainerProps> = props => {
    const someDataQuery = useData(
        someDataSource,
        // idle позволяет не отправлять никаких запросов за данными
        props.id ? { id: props.id } : idle
    );

    if (someDataQuery.isSuccess) {
        return (
            <SomeComponent data={someDataQuery.data} />
        );
    }

    return null;
};

export const SomeComponentContainer = React.memo(SomeComponentContainerPresenter);
```

Формат возвращаемого `useData` объекта такой же как у [useQuery](https://react-query.tanstack.com/reference/useQuery) в react-query.

#### Управление временем кеширования

По-умолчанию для данных используются настройки `staleTime: 7000` и `cacheTime: 30000`.

`staleTime` контроллирует сколько времени (в мс) считать данные свежими. Если данные не свежие - то они могут использоваться для отображения компонента, но при смене вкладок будут перезапрошены в фоновом режиме (при фоновом обновлении выставляется флаг `isRefetching: true`).

`cacheTime` контроллирует сколько времени (в мс) хранить в кеше несвежие данные.

Можно изменять настройки кеша для источника данных:

```typescript
export const someDataSource = dataSource<ISomeDataParams, ISomeData>({
    name: 'someData',
    fetch: fetchSomeData,
    options: {
        staleTime: Infinity,
        cacheTime: Infinity,
    },
});
```

#### Инвалидация

Инвалидация позволяет принудительно перезагрузить данные.

##### В самом компоненте

```typescript
const someDataQuery = useData(
    someDataSource,
    { id: props.id }
);

// для фонового обновления (флаг isRefetching)
someDataQuery?.refetch();
// для визуально отображаемой перезагрузки (флаг isLoading)
someDataQuery?.remove();
```

##### В произвольном месте

```typescript
ydoContainer.getInstance(DataManager)
    .invalidate(someDataSource, { id: 'test' });
```

##### Инвалидация по тегу

Иногда нужно инвалидировать не конкретный запрос, а группу запросов за данными. Это можно сделать с помощью тегов для источников данных.

```typescript
export const someDataSource = dataSource<ISomeDataParams, ISomeData>({
    name: 'someData',
    fetch: fetchSomeData,
    // функция принимает ISomeDataParams, что позволяет выставлять теги в зависимости от параметров запроса
    tags: _params => ['exampleTag'],
});

ydoContainer.getInstance(DataManager)
    // источник данных не указывается, инвалидируются запросы во всех источниках с таким тегом
    .invalidateTag('exampleTag');
```

#### Бесконечные запросы

Иногда нужно делать ленты с подгружаемыми элементами (например, лениво подгружаемые слоты на будущие даты).

```typescript
interface ITestParams {
    page: number;
}

interface ITestData {
    page: number;
    items: number[];
}

const testDataSource = infiniteDataSource<ITestParams, ITestData>({
    name: 'test',
    fetch: params =>
        new Promise<ITestData>(resolve => {
            setTimeout(() => {
                resolve({
                    page: params.page,
                    items: Array(10)
                        .fill(params.page * 10)
                        .map((value, index) => value + index),
                });
            }, 500);
        }),
    next: lastPage => ({ page: lastPage.page + 1 }),
});

const TestComponent = () => {
    const testQuery = useData(testDataSource, { page: 1 });

    if (testQuery.isSuccess) {
        return (
            <div>
                {testQuery.data.pages.map(page => (
                    <pre key={page.page}>
                        page #{page.page}: {JSON.stringify(page.items)}
                    </pre>
                ))}
                {testQuery.isFetchingNextPage ? <div>loading next page...</div> : null}
                <button onClick={() => testQuery.fetchNextPage()}>Next</button>
            </div>
        );
    }

    return null;
};
```
