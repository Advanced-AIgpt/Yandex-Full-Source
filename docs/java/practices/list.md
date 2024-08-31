### ArrayList vs LinkedList
Всегда используете ArrayList, кроме случаев когда вы четко понимаете зачем вам
[LinkedList](https://docs.oracle.com/javase/7/docs/api/java/util/LinkedList.html).
* LinkedList накладывает большой оверхед по памяти.
* За счет использования массивов `ArrayList` эффективно использует процессорные кеши при обращении к соседним элементам.
  Элементы `LinkedList` напротив разбросаны по памяти.
* ArrayList использует эффективный метод `System.arrayCopy()`.
* [Сравнение производительности](https://dzone.com/articles/arraylist-vs-linkedlist-vs#:~:text=LinkedList%20is%20implemented%20as%20a,on%20get%20and%20set%20methods.&text=Vector%20and%20ArrayList%20require%20space,of%20its%20size%20each%20time.)

В реальной жизни очень мало случаев к которых `LinkedList` был бы полезен. Возьмем к примеру код,
где нужно пройтись по списку и отфильтровать из него элементы. Может показаться, что это хороший пример для использования `LinkedList`.
На практике же куда эффективнее сделать новый список и переместить в него необходимые элементы.
Это будет быстрее и эффективнее по памяти.
