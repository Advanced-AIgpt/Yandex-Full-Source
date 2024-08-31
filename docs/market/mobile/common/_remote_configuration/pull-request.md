{% list tabs %}
 
- Через веб-интерфейс Арканума

    Нажать `Send to review`
    
    {% note info %}

    Не забывайте про читаемый commit message

    {% endnote %}

    ![](_assets/arcadia_diff.png)
    
    Опубликовать созданный пр (нажать кнопку `Publish`)

- Через Аркадию 
 
    ```bash
    arc add .
    arc commit -m 'BLUEMARKETAPPS-12345: Added toggle'
    arc push --set-upstream users/{staff login}/BLUEMARKETAPPS-12345
    arc pr create
    ```

    Затем открыть пр по ссылке, которую выведет команда `arc pr create`, и опубликовать его (нажать кнопку `Publish`).

{% endlist %}
