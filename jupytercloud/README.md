# JupyterCloud

- [Main page](https://jupyter.yandex-team.ru)
- [Service Documentation](https://wiki.yandex-team.ru/jupyter/docs/)
- [Support chat](https://nda.ya.ru/3UYxBK)

---

`backend/` directory is the main place for code that extends JupyterHub for Yandex infrastructure.

`infrastructure/` contains various files needed to use JupyterCloud in production (deploy
 templates, salt states, etc).

`nbextensions/` and `labextensions/` contain small and helpful Yandex-only Jupyter extensions.

`arcadia_kernel/` contains Jupyter Arcadia Kernel, which can be used in our service.

`tools/` contains some small helpful utilities for Jupyter installation admins.
