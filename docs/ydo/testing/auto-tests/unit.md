# Unit

- [Официальная документация Jest](https://jestjs.io/docs/getting-started)
- [Паттерн Arrange-Act-Assert](http://wiki.c2.com/?ArrangeActAssert)

Запустить юниты в одном файле:

```bash
npm run unit src/путь/до/теста
```

## RTL

React-testing-library – это замена Enzyme с возможностью писать тесты на хуки.
Так что если хотите написать тест на компонент или хук, используйте RTL.

Почитать о том, как писать тесты с помощью RTL, основные принципы и примеры, можно в [официальной документации](https://testing-library.com/docs/react-testing-library/intro/).
Здесь же будут описаны некоторые тонкости и решения типовых задач / проблем.

Дополнительно, к Jest подключены кастомные матчеры для DOM. Почитать о них можно [здесь](https://github.com/testing-library/jest-dom).

### Использование act

Все действия, которые могут вызвать ререндеры, нужно оборачивать в функцию `act`.

```tsx
act(() => {
    // Вызов ивентов через fireEvent.*
    // Вызов колбэков вручную или через, например, jest.runAllTimers()
    // Вызов ивентов напрямую через DOM
});
```

{% note info %}

Возможно, тест будет работать и без обертки `act`, но лучше ее добавить. Так тест будет работать стабильнее при переходе не следующую версию React.

{% endnote %}

### Селект DOM элементов

Следуя принципу ["Чем больше тесты похожи на сценарий использования, тем больше уверенности они дают"](https://testing-library.com/docs/guiding-principles), RTL предлагает использовать специальные [селекторы](https://testing-library.com/docs/queries/about), которые позволяют получать элементы по их семантическим свойствам, например, `getByRole` или `getByText`, а не по классу.

Если все же нет возможности заиспользовать эти селекторы, то можно обратиться к `querySelector`.

Пример:

```tsx
const { container } = render(<Component />);

container.querySelector('.Block-Elem');
```

### Тестирование хуков

Для тестирования хуков используется пакет `@testing-library/react-hooks`.
Он содержит функцию `renderHook`, с помощью которой можно отрендерить хук в пустом компоненте и получить его результат.
Больше информации можно найти в [документации RTL](https://testing-library.com/docs/react-testing-library/api#renderhook).

Вот [пример](https://a.yandex-team.ru/arcadia/frontend/services/ydo/src/features/order-customizer/components/newForms/Questions/QuestionGeo/hooks/__tests__/useQuestionGeoInitialValue.test.tsx) теста хука.

### Решение типовых задач / проблем

#### Сравнение снапшотов

```tsx
const { asFragment } = render(<Component />);

expect(asFragment()).toMatchSnapshot();
```

{% note info %}

Функция `asFragment` в отличие от `shallow` возвращает снапшот уже с финальной версткой (div, span и прочее), без какого-либо упоминания компонентов, так как RTL производит честный рендер, а не shallow.

{% endnote %}

#### Изменение пропсов компонента

```tsx
const { rerender } = render(<Component prop={1} />);

rerender(<Component prop={2} />);
```

#### Проверка на пустой DOM элемент

```tsx
const { container } = render(<Component />);

expect(container).toBeEmptyDOMElement();
```
