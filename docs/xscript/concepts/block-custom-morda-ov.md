# Custom-morda-блок

Блок **Custom-morda** содержит функциональность по обработке персональных настроек пользователя, хранящихся в куке _my_.

**Пример блока Custom-morda**:

```
<custom-morda>
   <method>save_tune</method>
   <param type="String">block,city_id,count</param>
   <param type="Long">27</param>
</custom-morda>
```

В приведенном примере значения перечисленных переменных из контейнера [State](state-ov.md) устанавливаются в 27-ой блок куки my.

### Узнайте больше {#learn-more}
* [Методы блока custom-morda](../appendices/block-custom-morda-methods.md)
* [custom-morda](../reference/custom-morda.md)