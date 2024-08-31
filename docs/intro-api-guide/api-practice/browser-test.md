# Создание тестового запроса в строке браузера

Если API-ключ передается в строке запроса, вы можете выполнить запрос в поисковой строке браузера. Например, сделайте тестовый вызов [метода complete](https://yandex.ru/dev/predictor/doc/dg/reference/complete.html) API Предиктора:

1. В поисковой строке браузера введите запрос:
    ```html
    https://predictor.yandex.net/api/v1/predict/complete?q=wh&lang=en&key=ваш-API-ключ
    ```
    
1. На странице браузера в ответе вернется:
   ```xml
    <CompleteResponse endOfWord="false" pos="-2">
      <text>
        <string>which</string>
      </text>
    </CompleteResponse>
    ```
    

