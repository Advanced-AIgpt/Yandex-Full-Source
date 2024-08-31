# Методы Mobile-блока

**Список методов Mobile-блока**:
- [detect_phone](block-mobile-methods.md#detect_phone).

#### detect_phone (detectPhone) {#detect_phone}

Возвращает информацию о мобильном телефоне в [заданном формате](http://wiki.yandex-team.ru/StepanMoroz/mobile-block-xscript):

```
<model name="6233" vendor="Nokia">
     <attr name="app_fotki" value="1"/>
     <attr name="app_mail" value="1"/>
     <attr name="app_maps" value="1"/>
     <attr name="app_metro" value="1"/>
     <attr name="app_operamini" value="1"/>
     <attr name="app_yandex" value="0"/>
     <attr name="cam_access" value="1"/>
     <attr name="canring" value="0"/>
     <attr name="certificate_prefix" value="t"/>
     <attr name="device_class" value="midp2hsg"/>
     <attr name="device_class_desc" value="Java MIDP2 (huge + bluetooth)"/>
     <attr name="device_id" value="3734"/>
     <attr name="family_id" value="13"/>
     <attr name="family_name" value="nokia-hs"/>
     <attr name="fs_access" value="1"/>
     <attr name="heapsize" value=""/>
     <attr name="iconsize" value="64x64"/>
     <attr name="is_bt" value="1"/>
     <attr name="is_pda" value=""/>
     <attr name="screenx" value="240"/>
     <attr name="screeny" value="320"/>
     <attr name="symbolic" value="1"/>
     <attr name="touchflo" value="0"/>
     <attr name="vendor_id" value="200"/>
</model>
```

Если мобильный телефон не определен, в том числе в случае запроса с персонального компьютера, будет возвращено следующее сообщение об ошибке:

```
<error>Unknown user agent and wap profile</error>
```

**Входные параметры**: нет.

**Пример использования**:

```
<mobile-block method="detect_phone"/>
```

### Узнайте больше {#learn-more}
* [Mobile-блок](../concepts/block-mobile-ov.md)
* [mobile-block](../reference/mobile-block.md)
* [Описание формата данных о мобильном телефоне](https://wiki.yandex-team.ru/yandexmobile/wap/phonedetect/techinfo)