# Задача YA_EXEC

Тип задачи `YA_EXEC` позволяет собирать и запускать пользовательские программы (PROGRAM, PY2_PROGRAM, etc.), описанные сборкой ya make, в Sandbox.

## Опции
* **Svn url for arcadia**
Путь до Аркадии в формате arcadia:/arc/(branches/some/branch,trunk)/arcadia.
Нужно учитывать, что указывая бранч, все сборочные инструменты будут также браться из бранча; также скорее всего потребуется несколько больше времени на чекаут ветки, так как она редко запрашивается и не будет в кеше sandbox.
По умолчанию: arcadia:/arc/trunk/arcadia

* **Definition flags**
Флаги сборки, например, `-DUSE_ARCADIA_PYTHON=no -DkeyX=valX`

* **Program to build and run**
Путь к проекту относительно корня Аркадии, дополненный именем программы, прописанным в ya.make проекта.
  
* **Args to program**
Аргументы, которые будут переданы запускаемой программе. Агрументы могут содержать переменные окружения, описаные в соответствующем поле.
  
* **Environment variables**
Переменные окружения, с которыми запустится программа.

  Может быть использовано вместе с Vault:
  `VAR_NAME=$(vault:value:owner:name)` или `VAR_NAME=$(vault:file:owner:name)`

* **Post Execution Script**

* **Share directory as resource**

* **Use DNS64**

