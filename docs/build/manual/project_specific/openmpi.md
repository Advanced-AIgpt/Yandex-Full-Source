# Специфика сборки с OpenMPI

## Разработка

Примеры приложений на C и C++: [https://a.yandex-team.ru/arc/trunk/arcadia/contrib/libs/openmpi/examples](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/libs/openmpi/examples)

Пример приложения на Python: [https://a.yandex-team.ru/arc/trunk/arcadia/contrib/python/mpi4py/ut/app](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/libs/openmpi/examples)

## Запуск

Для запуска mpi-приложений нужны:
```
contrib/libs/openmpi/orte/tools/orterun/orterun
contrib/libs/openmpi/orte/tools/orterun/orted
```
`orterun`, `mpirun` и `mpiexec` в OpenMPI это одно и то же.

Собирайте ваше приложение, `orterun` и `orted` из одной ревизии! Приложение может запуститься с системным или старым OpenMPI, но это не гарантировано.
Приложение запускается командой `/…/bin/orterun path/to/app`. При этом важно, чтобы:
- путь к `orterun` был абсолютным, иначе будет использован `orted` из `$PATH`
  <small>В shell можно писать `~/bin/orterun` или `$HOME/bin/orterun`.</small>
- `orterun` запускался из `bin`, иначе будет использован `orted` из `$PATH`
  <small>Вместо реальной поддиректории `bin` можно создать в директории с `orterun` симлинк `ln -s . bin`.</small>
- `orted` был в той же директории, что и `orterun`
- `orted` и ваше приложение были доступны на всех хостах по одному и тому же абсолютному пути

Например, можно делать так:
```sh
ya make --no-src-links -I ~/myapp/bin contrib/python/mpi4py/ut/app/py2 contrib/libs/openmpi/{ompi/tools/ompi_info,orte/tools/{orterun,orted}}
rsync -a --delete ~/myapp/ otherhost:myapp/
~/myapp/bin/orterun -H localhost -H otherhost ~/myapp/bin/mpi4py-ut-app-py2 -v
```

Для локального запуска без `orterun` нужно сохранить в переменной окружения `OPAL_BINDIR` директорию с `orted`:
```sh
ya make contrib/libs/openmpi/orte/tools/orted contrib/python/mpi4py/ut/app/py2
OPAL_BINDIR=contrib/libs/openmpi/orte/tools/orted contrib/python/mpi4py/ut/app/py2/mpi4py-ut-app-py2
```

## Настройка

`contrib/libs/openmpi/ompi/tools/ompi_info` с опцией `-a` печатает все возможные параметры.

Так можно включить максимальное логгирование:
```sh
mkdir -p ~/.openmpi/
ompi_info -a --parseable | grep -o '[^:]*_verbose' | grep -v mca_verbose | uniq | xargs -I{} echo '{} = 100' > ~/.openmpi/mca-params.conf
```

## Тестирование

Примеры использования в тестах: [https://a.yandex-team.ru/arc/trunk/arcadia/contrib/python/mpi4py/ut/test.py](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/python/mpi4py/ut/test.py)

Как перезапустить pytest под mpi из кода теста: [https://ml.yandex-team.ru/thread/devtools/169447935979889387/#message169729410956600223](https://ml.yandex-team.ru/thread/devtools/169447935979889387/#message169729410956600223)

В CI стоит включать:

- `mpi_yield_when_idle` :: освобождать процессор во время ожидания события
- `rmaps_base_oversubscribe` :: разрешить запускать больше процессов чем дано процессоров

и выключать `rsh` ([так](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/python/mpi4py/ut/test.py?rev=5296599#L16) или [так](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/python/mpi4py/ut/test.py?rev=5296599#L27)), чтобы избежать следующей ошибки из-за отсутствия ssh в PATH:
```
The value of the MCA parameter "plm_rsh_agent" was set to a path that could not be found:
plm_rsh_agent: ssh : rsh
```
