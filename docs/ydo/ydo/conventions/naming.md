# Соглашения об именовании

### Структура файлов React компонента (внутри директории `./scr/components/`)
`./Block/Block.tsx` - компонент-блок\
`./Block/Block@desktop.tsx` - реализация компонента для десктопной платформы\
`./Block/Block.scss` - стили компонента\
`./Block/_modName/Block_modName_modVal.tsx` - модификатор компонента-блока\
`./Block/-Elem/Block-Elem.tsx` - компонент-элемент\
`./Block/-Elem/_modName/Block-Elem_modName_modVal.tsx` - модификатор компонента-элемента\
`./index.ts` - модуль блока, экспортищующий компонент, динамически определяющий платформу (withPlatform)\
`./Block/constants.ts` - модуль с контстантами, использующихся в компонентах данного блока\
`./Block/types.ts` - типы, использующиеся в компонентах данного блока\
`./Block/containers/index.ts` - контейнерные компоненты, созданные на базе данного блока (react-redux)\
`./Block/selectors/<selector_name>.ts` - селекторы (`reselect`), использующиеся только на уровне данного блока\
`./Block/tests/Block.test.tsx` - unit тест компонента\
`./Block/tests/Block.story.tsx` - storybook компонента\
`./Block/tests/Block.common.hermione.js` - hermione тест компонента\

### Именование компонентов

* Из common уровня всегда экспортить компонент с суффиксом Common (ComponentCommon)
* Из desktop - с суффиксом Desktop (ComponentDesktop),
  ```js
      import { ComponentCommon } from 'Component';

      import './Component@desktop.pcss';

      export { ComponentCommon as ComponentDesktop };
  ```
* Из touch - с суффиксом Touch (ComponentTouch)
  ```js
      import { ComponentCommon } from 'Component';

      import './Component@toucn.pcss';

      export { ComponentCommon as ComponentToucn };
  ```
* Из index экспортим либо компонент из withPlatform, либо реэкспорт из common уже без Common (Component)
  ```js
      import { ComponentDesktop } from 'Component@dekstop';
      import { ComponentTouch } from 'Component@Touch';

      export const Component = withPlatform(ComponentDesktop, ComponentTouch);
  ```
  или
  ```js
      import { ComponentCommon } from 'Component';

      export { ComponentCommon as Component };
  ```

Такой подход именования позволит избежать ситуаций, когда вместо компонента из `index.ts` импортируется CommonComponent.

При чистом реэкспорте ComponentCommon как платформенного, платформенный файл создавать не нужно, можно импортировать напрямую с Common уровня в индексный файл компонента.
