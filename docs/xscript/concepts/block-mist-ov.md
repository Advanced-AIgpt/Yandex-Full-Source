# Mist-блок

_Mist-блок_ предназначен для выполнения операций над данными контейнера [State](state-ov.md).

Mist-блоки позволяют создавать переменные в контейнере State, что дает возможность обмена данными между CORBA-компонентами, обращения к которым производятся с помощью [CORBA-блоков](block-corba-ov.md).

**Пример Mist-блока**:

```
<mist>
   <method>set_state_domain</method>
   <param type="String">tmp</param>
   <param type="String">http://www.yandex.ru/</param>
   <param type="Long">2</param>
</mist>
```

## Сравнение с CORBA-сервантом Mist {#mist-block-improvement}

Рекомендуется по возможности применять Mist-блоки вместо CORBA-вызовов серванта Mist, так как это позволяет:

- уменьшить нагрузку на front-end-ы, поскольку гораздо "дешевле" выполнить код Mist-блока локально, чем сделать CORBA-вызов;
- уменьшить нагрузку на back-end-ы;
- уменьшить нагрузку на сеть;
- уменьшить время обработки запроса;
- уменьшить зависимость XScript-а от удаленного компонента Mist.

### Узнайте больше {#learn-more}
* [Методы Mist-блока](../appendices/block-mist-methods.md)
* [mist](../reference/mist.md)
