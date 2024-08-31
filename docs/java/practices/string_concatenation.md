### Конкатенация строк
* Для любых простых конкатенаций строк (без циклов и if'ов) следует использовать оператор (+)
    * Использование StringBuilder'а усложнит код и уменьшит читаемость. Производительность при этом не пострадает,
      т.к компилятор за нас использует `StringBuilder`
      ([1](https://dzone.com/articles/string-concatenation-performacne-improvement-in-ja#:~:text=To%20increase%20the%20performance%20of,by%20evaluation%20of%20an%20expression.),
      [2](https://pellegrino.link/2015/08/22/string-concatenation-with-java-8.html),
      [3](https://medium.com/javarevisited/java-compiler-optimization-for-string-concatenation-7f5237e5e6ed)).
* `StringBuffer` это синхронизированный аналог `StringBuilder`. Его не стоит использовать без явной необходимости,
  т.к. это скажется на производительности. (Effective Java, 3rd edition, Item 93). Синхронизацию обычно делают где-то снаружи.
