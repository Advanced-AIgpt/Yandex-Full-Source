# Как определить город пользователя

Ниже приведен код, позволяющий определить город пользователя с учетом его IP-адреса, заголовка _X-Forwarded-For_, портальных настроек (кука _yandex_gid_) и настроек сервиса (кука _my_), а затем преобразовать полученный id региона Геобазы в id региона, использующийся на сервисе Яндекс.Погода.

```
<mist method="set_state_long">
     <param type="String">user-city-id</param>
     <param type="String">27612</param>
</mist>

<!-- Получение города из пользовательских настроек -->
<custom-morda method="set_state_by_tune">
     <param type="String">param,city-tune</param>
     <param type="Long">22</param>
</custom-morda>

<mist method="set_state_long">
     <param type="String">city-tune</param>
     <param type="StateArg">city-tune</param>
</mist>

<!-- Получение города по IP-адресу пользователя -->
<geo-block method="set_state_region" guard-not="city-tune">
     <param type="String">region</param>
</geo-block>

<geo-block method="set_state_parents" guard-not="city-tune">
     <param type="String">all_region</param>
     <param type="StateArg" default="213">region</param>
</geo-block>

<!-- Получение id, использующегося в сервисе Яндекс.Погода -->
<block name="Yandex/Project/Weather.id" method="dump_by_regions" tag="3600" guard-not="city-tune">
     <param type="String">city-geo</param>
     <param type="StateArg" default="213">all_region</param>
     <xpath expr="/state" result="city-geo" /> 
</block>

<!-- Окончательное определение города пользователя -->
<mist method="set_state_long" guard="city-geo">
     <param type="String">user-city-id</param>
     <param type="StateArg">city-geo</param>
</mist>

<mist method="set_state_long" guard="city-tune">
     <param type="String">user-city-id</param>
     <param type="StateArg">city-tune</param>
</mist>
```

### Узнайте больше {#learn-more}
* [Понятие «Город по умолчанию» или Алгоритм определения региона пользователя](https://wiki.yandex-team.ru/DefaultCity)
* [Geo-блок](../concepts/block-geo-ov.md)
* [Mist-блок](../concepts/block-mist-ov.md)
* [Методы Geo-блока](../appendices/block-geo-methods.md)
* [Методы Mist-блока](../appendices/block-mist-methods.md)
* [XML-файл на XScript](../concepts/xscript-file-ov.md)