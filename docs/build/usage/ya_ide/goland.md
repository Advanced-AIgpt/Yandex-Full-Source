# ya ide goland : генерация проекта для JetBrains Goland

*Документации пока нет*

```
Usage: ya ide goland [OPTION]... [TARGET]...
Generate stub for Goland

Options:
  Bullet-proof options
    -h, --help          Print help
    -T=PROJECT_TITLE, --project-title=PROJECT_TITLE
                        Custom IDE project title (default: arcadia)
    --dirname-as-project-title
                        Use cwd dirname as default IDE project title
    -P=PROJECT_OUTPUT, --project-output=PROJECT_OUTPUT
                        Custom IDE project output directory


  Advanced options
    --make-src-links    Create ya make symlinks in source tree
    --dist              Use distbuild
    --make-args=YA_MAKE_EXTRA
                        Extra ya make arguments
    --with-go-modules   Enable go modules support
```
## Дополнительные ссылки
- [Go + Arcadia](https://docs.yandex-team.ru/arcadia-golang/getting-started)
