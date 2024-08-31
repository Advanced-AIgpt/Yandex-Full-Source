# Практическое задание с Postman

## Что нужно сделать
Создайте два запроса в Postman на определение текущей погоды и сравните полученные значения.

## Как это сделать

1. Убедитесь, что получили [нужные API-ключи](../api-practice/authorization.md).
1. Выберите один любой город в России. Не берите те значения, которые указаны в этом документе или в Справке API Яндекс&#160;Погоды.
1. Сделайте два GET-запроса на определение погоды в выбранном городе по аналогии с [тестовыми](../api-practice/postman-test.md):
    - к API [Яндекс&#160;Погоды](https://yandex.ru/dev/weather/doc/dg/concepts/forecast-info.html)
    - к API [OpenWeatherMap](https://openweathermap.org/current).
    
1. Сравните полученные значения о текущей погоде по параметрам, которые есть в ответах обоих запросов. Их имена могут отличаться. Определите, какой нужно выбрать объект для сравнения в API Яндекс&#160;Погоды. Всего общих параметров в вашем запросе может быть от 6 до 9. Постарайтесь найти хотя бы 5.
    
1. Выпишите списком или таблицей названия общих параметров и расхождения в полученных значениях.
2. Приложите JSON-файлы ответов на два запроса.

## Ожидаемый результат в комментариях

- Два JSON-файла с ответами на запросы.
- В свободной форме список параметров с отличающимися значениями.
