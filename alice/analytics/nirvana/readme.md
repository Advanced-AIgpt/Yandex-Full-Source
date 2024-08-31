Файлы скопированы из репы Нирваны, т.к. нормальная установка у них только через Аркадию.
Как пользоваться: https://wiki.yandex-team.ru/jandekspoisk/nirvana/manual/nirvana-python-lib/
Если коротко, для эмуляции окружения Нирваны, нужно:
1. положить рядом со своим скриптом файлик `job_context.json`, в который записать параметры и пути к файлам с входами и выходами. Пример лежит рядом с этим readme: `sample_job_context.json`.
2. Накачать из Нирваны, или нагенерить иным образом исходных данных для input'ов.
3. В коде, вместо парсинга командной строки, сделать:

```python
    from nirvana.job_context import context
    ctx = context()
    inputs = ctx.get_inputs()
    outputs = ctx.get_outputs()
    params = ctx.get_parameters()
```

...и получить словарики с соответствующими настройками. Значения `inputs` и `outputs`, если они не множественные, лучше получать через метод `.get()`. Значения `params` можно обычным синтаксисом словаря.

```python
    some_json_input = json.load(open(inputs.get('some_json_input')))
    some_param = params['some_param']
```
