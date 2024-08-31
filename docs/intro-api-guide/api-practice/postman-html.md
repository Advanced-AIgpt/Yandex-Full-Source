# Создание HTML‑страницы с помощью Postman

{% note info %}

Это информация для углубленного понимания.

{% endnote %}

API-ответы можно обрабатывать, чтобы создавать из них HTML-документы. На этой странице вы можете попробовать создать собственную страницу, которая будет отображать информацию из запроса. Это не требуется для погружения, но пригодится для углубленного понимания.

Здесь приведен шаблон для HTML-страницы, которая будет выводить JSON-ответ [запроса текущей погоды OpenWeatherMap](../api-practice/postman-test.md#openweather).

Как будет выглядеть страница с выводом трех полей:

![](../images/html-example.png)

## Пошаговое создание файла {#step-by-step}

1. Создайте в текстовом редакторе файл `open-weather-map.html` и вставьте в него шаблон с основными элементами и ссылкой на jQuery.
    ```html
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset="UTF-8">
      <script src="https://ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js"></script>
      <title>Пример HTML-страницы</title>
      <script>
        ЗДЕСЬ БУДЕТ КОД ИЗ POSTMAN
      </script>
    </head>
    <body>
      <h1>Пример HTML-страницы с результатами запроса</h1>
      <div id="currentWeather">Текущая погода: </div>
    </body>
    </html>
    ```
    
1. Выберите в Postman запрос текущей погоды и нажмите значок ![](../images/code.png). Выберите **JavaScript - jQuery** и скопируйте код.
1. Вставьте код в элемент `script`, вместо строчки «ЗДЕСЬ БУДЕТ КОД ИЗ POSTMAN».
1. Под блоком кода
    ```js
    $.ajax(settings).done(function (response) {
    console.log(response);
    ```
    добавьте строчки, которые будут описывать погоду:
    ```js
    var currentWeather = response.weather[0].description;
    $("#currentWeather").append(currentWeather);
    ```
    
1. Сохраните html-файл и откройте его в браузере. Вам отобразится строка «Текущая погода» с описанием погоды по запрошенному городу, но можно вывести и другие поля. Например, в приведенном выше примере еще выводятся название города и скорость ветра с помощью следующих строк:
    
    ```js
    var name = response.name;
    $("#name").append(name);
    
    var windSpeed = response.wind.speed;
    $("#windSpeed").append(windSpeed);
    ```
    
    Элементы `div` для выведения этих параметров попробуйте добавить самостоятельно или посмотрите в коде ниже.

## Готовый код страницы {#ready}

Не забудьте добавить свой API-ключ.

```html
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <script src="https://ajax.googleapis.com/ajax/libs/jquery/
1.11.1/jquery.min.js"></script>
  <title>Пример HTML-страницы</title>
  <script>
    var settings = {
      "async": true,
      "crossDomain": true,
      "url": "https://api.openweathermap.org/data/2.5/weather?q=Kaliningrad&
      units=metric&lang=ru&appid=ваш-API-ключ",
      "method": "GET",
      "timeout": 0,
    };

    $.ajax(settings).done(function (response) {
      console.log(response);

      var name = response.name;
      $("#name").append(name);

      var windSpeed = response.wind.speed;
      $("#windSpeed").append(windSpeed);

      var currentWeather = response.weather[0].description;
      $("#currentWeather").append(currentWeather);

    });
  </script>
</head>
<body>
  <h1>Пример HTML-страницы</h1>
  <div id="name">Город: </div>
  <div id="windSpeed">Скорость ветра: </div>
  <div id="currentWeather">Текущая погода: </div>
</body>
</html>
```

