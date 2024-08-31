# Сервис тыквы для Моей Алисы

Окружение в Y.Deploy: https://deploy.yandex-team.ru/projects/my_alice

Кеширует успешные ответы HTTP-адаптера и отвечает ими аппхосту.
Используется как fallback в случае фейла источника TEMPLATES.

# Локальный запуск

## IDEA

Проект генерируется вместе с основным бекендом "Моей Алисы":
```
cd $ARCADIA/alice/paskills/my_alice
./gen_project.sh
```

## ya.make

```shell script
ya make && ./my_alice_pumpkin.sh --debug
```

`stdout` будет перенапрален в файл `my_alice_pumpkin.log`
