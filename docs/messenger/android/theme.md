# Цветовая тема
Для установки цветовой темы достаточно передать провайдер ресурса темы в `MessagingConfiguration` (параметр `themeOverlayProvider`). Применится при следующем запуске активити мессенджера.
Доступные темы:

- **`Messaging.ThemeOverlay`** – светлая (установлена по умолчанию)

- **`Messaging.ThemeOverlay.Dark`** – темная

- **`Messaging.ThemeOverlay.DayNight`** – зависит от ночного мода на устройстве: `Messaging.ThemeOverlay` в дневном режиме и `Messaging.ThemeOverlay.Dark` – в ночном


### Attachments
Для использования цветов мессенджера в ` AttachmentsHost#overrideUiConfiguration` используйте класс `MessengerAttachmentsUiConfiguration`.
В данном случае, будет использоваться тема, указанная в `themeOverlayProvider` (`MessengerHost.configuration.messagingConfiguration.themeOverlayProvider`).

### Кастомная тема
Для создания кастомной темы нужно:

- создать стиль наследник `Messaging.ThemeOverlay` (если тема светлая) или `Messaging.ThemeOverlay.Dark` (если тема темная). Родителя важно выбрать, т.к. базовая тема определяет цвет иконок статусбара и навбара, страницу паспорта, лого.

- переопределить атрибуты из доступной палитры (см. ниже)

- прокинуть в `MessagingConfiguration` ресурс созданной темы

### Палитра
Название в палитре | .defaultLight | .defaultDark | Описание
:--- | :--- | :--- | :--- 
messagingCommonBackgroundColor | #ffffff | #000000| Основной фоновый цвет, используется на списке чатов, внутри чата как фон для переписки
messagingCommonBackgroundTransparentColor | #00ffffff | #00000000| Используется для градиентов
messagingCommonBackgroundTransparent50PercentColor | #7bffffff | #7b000000| –
messagingCommonBackgroundSecondaryColor | #F5F5F5 | #1A1A1A| Дополнительный фоновый цвет, используется, как правило, поверх элементов с цветом основного фона: подложка баннеров, плейсхолдер иконки, скелетоны
messagingCommonTextPrimaryColor | #000000 | #EEEEEE | Основной цвет текста: названия чатов, заголовки экранов, текст в элементах списка
messagingCommonTextSecondaryColor | #999999 | #888888| Второстепенный цвет текста: статусы, пояснения. Используется, как правило, в конструкциях где уже есть основной текст 
 messagingCommonTextSecondaryTransparent50PercentColor | #7b999999 | #7b888888| – 
 messagingCommonDividerColor | #EDEDED | #1F1F1F| Цвет для разнообразных разделителей 
 messagingCommonSettingsBackgroundColor | #F5F5F5 | #000000| Цвет фона экранов с секциями настроек, если нужно цветом отделить секцию от фона 
 messagingCommonSettingsItemColor | #ffffff | #121212| Цвет элементов списка внутри секции настроек,  если нужно цветом отделить секцию от фона 
 messagingCommonActionbarColor | #ffffff | #0D0D0D| Цвет разнообразных тулбаров. Как правило, такие тулбары расположены поверх основного фона `common/bg` 
 messagingCommonAccentColor | #2ABCBC | #51B5B7| Акцентный цвет 
 messagingCommonAccentFgColor | #ffffff | #000000| Цвет шрифтов и иконок, которые находятся на подложке из messagingCommonAccentColor. `C Alicekit 128.0`
 messagingCommonAccentTransparent10PercentColor | #192ABCBC | #1951B5B7| – 
 messagingCommonAccentTransparent25PercentColor | #402abcbc | #4051B5B7| – 
 messagingCommonAccentTransparent50PercentColor | #802ABCBC | #8051B5B7| – 
 messagingCommonAccentTextColor | #00A8A8 | #51B5B7| Акцентный цвет для текстовых кнопок 
 messagingCommonOnlineColor | #2DCBCB | #51B5B7 | Цвет точки онлайновости 
 messagingCommonDestructiveColor | #F65151 | #FF564C| Цвет деструктивных действий: выход, удаление и тп 
 messagingCommonCounterColor | #00A8A8 | #EEEEEE| Цвет активных каунтеров 
 ~~messagingOnSurfaceColor~~ | #ffffff | #000000| Цвет шрифтов и иконок, которые находятся на подложке из messagingCommonAccentColor  `@Deprecated`
 **Иконки** 
 messagingCommonIconsPrimaryColor | #000000 | #EEEEEE | Основной цвет иконок, обычно такие иконки вызывают переходы на другой экран 
 messagingCommonIconsSecondaryColor | #A6A6A6 | #777777| Второстепенный цвет иконок, используется в интерфейсах, где нет перехода на другой экран, но есть изменения на текущем экране: переключение между категориями эмодзи, вызов панели стикеров и тп 
 messagingCommonIconsSecondaryTransparent50PercentColor | #7bA6A6A6 | #7b777777| – 
 messagingCommonIconsSecondaryTransparent75PercentColor | #bfA6A6A6 | #bf777777| – 
 **Входящее сообщение** 
 messagingIncomingBackgroundColor | #F2F2F2 | #141414| Цвет баббла входящего сообщения 
 messagingIncomingPrimaryColor | #000000 | #EEEEEE| Основной цвет для текста входящего
 messagingIncomingSecondaryColor | #999999 | #777777| Второстепенный цвет для текста/иконок/времени отправки, статуса и тп 
 messagingIncomingSecondaryTransparent50PercentColor | #7b999999 | #7b777777| – 
 messagingIncomingLinkColor | #6682CC | #69A5D4| Цвет ссылки
 messagingIncomingButtonColor | #ffffff | #0D0D0D| Цвет подложки под иконкой для баблов голосовых, файлов и тп 
 **Иходящее сообщение** 
 messagingOutgoingBackgroundColor | #DDF2F4 | #003333| Цвет баббла исходящего сообщения 
 messagingOutgoingPrimaryColor | #000000 | #EEEEEE| Основной цвет для текста исходящего 
 messagingOutgoingSecondaryColor | #0A97A7 | #478485| Второстепенный цвет для текста/иконок/времени отправки, статуса и тп 
 messagingOutgoingSecondaryTransparent50PercentColor | #7b0A97A7 | #7b478485| – 
 messagingOutgoingLinkColor | #6682CC | #6682CC| Цвет ссылки 
 messagingOutgoingButtonColor | #ffffff | #001F1F| Цвет подложки под иконкой для баблов голосовых, файлов и тп 
 **Поле ввода** (с Alicekit 106.0) 
 messagingChatSendIconColor | #ffffff | #0D0D0D| Цвет иконки отправки сообщения 
 messagingChatSendIconBackgroundColor | #2ABCBC | #51B5B7| Цвет фона иконки отправки сообщения 
 messagingChatInputBackgroundColor | #F5F5F5 | #1A1A1A|  Цвет фона поля ввода 
 messagingChatInputTextColor | #000000 | #EEEEEE | Цвет текста поля ввода 
 messagingChatInputHintColor | #999999 | #777777| Цвет hint текста поля ввода 
 **Опросы** (с Alicekit 125.0) 
messagingPollsBackgroundColor | #d9ffffff | #7b478485 | Цвет фона для опросов (по сути, цвет фона у прогрессбаров вариантов ответов)
