# Arcadia C++ library guide

* Мы не используем boost.
* Ограниченно используем STL. Если в `util` есть аналог функции или класса из `std::`, следует использовать его.
  * `std::string` -> `TString`,
  * `std::streams` -> `TInputStream`, `TOutputStream`
  * `thread_local`- переменные по-разному инициализируются на разных платформах ([что можно с этим сделать](https://docs.yandex-team.ru/arcadia-cpp/cookbook/std#c++-rantajm)).
  * `static` (переменные) -> `Singleton`
      * `Singleton<>` позволяет управлять порядком инициализации и уничтожения.
      * Порядок уничтожения между всеми `static T` и всеми `Singleton<>` не определён.
        В Аркадии (по крайней мере, в общих библиотеках) мы используем `Singleton<>`, поэтому везде тоже нужно использовать `Singleton<>`, иначе возможны очень странные падения.
      * `Singleton<>` быстрее.

  * `unique_ptr` -> `THolder`
  * `shared_ptr` -> `TAtomicSharedPtr` (потокобезопасный счётчик), `TSimpleSharedPtr` (потоконебезопасный счётчик)
  * `optional` -> `TMaybe`
