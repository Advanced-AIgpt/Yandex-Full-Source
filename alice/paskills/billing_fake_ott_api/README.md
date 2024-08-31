# Запуск

```
ya make
./billing-fake-ott --config config.json --port 8080
```

# Конфиг

## Пример

```
{
    "content_available": {
        "delay": {
            "type": "fixed",
            "value": 10000
        }
    },
    "content_options": {
        "delay": {
            "type": "random",
            "mean": 10000,
            "std": 500
        }
    }

}
```