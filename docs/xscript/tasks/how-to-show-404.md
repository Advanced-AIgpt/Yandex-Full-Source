# Как показать страницу 404

Ниже приведен код, позволяющий показать страницу 404.

Сама страница 404 (404.xml):

```
<?xml version="1.0" encoding="UTF-8"?>
<container xmlns:x="http://www.yandex.ru/xscript">
    <x:mist method="attachStylesheet" xpointer="/..">
        <x:param type="String">/usr/local/www/common/404/404.xsl</x:param>
    </x:mist>
   
    <x:mist method="setStatus" xpointer="/..">
        <x:param type="String">404</x:param>
    </x:mist>
</container>
```

Код, вызывающий страницу 404.xml (если она лежит в корневой директории):

```
<file method="invoke" guard="error">
    <param type="String">docroot:///404.xml</param>
</file>
```

### Узнайте больше {#learn-more}
* [File-блок](../concepts/block-file-ov.md)
* [Mist-блок](../concepts/block-mist-ov.md)
* [Методы File-блока](../appendices/block-file-methods.md)
* [Методы Mist-блока](../appendices/block-mist-methods.md)
* [XML-файл на XScript](../concepts/xscript-file-ov.md)