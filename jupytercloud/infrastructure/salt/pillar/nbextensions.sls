nbextensions_packages:
- widgetsnbextension
- jupyter_nbextensions_configurator
- jupyter_contrib_nbextensions
- nbresuse
- ipyleaflet

nbextensions_activate_commands:
  jupyter_nbextensions_configurator: "jupyter-nbextensions_configurator enable --sys-prefix"
  jupyter_contrib_nbextensions: "jupyter contrib nbextension install --sys-prefix"

nbextensions_to_enable:
- [code_prettify/code_prettify, jupyter_contrib_nbextensions]
- [execute_time/ExecuteTime, jupyter_contrib_nbextensions]
- [codefolding/main, jupyter_contrib_nbextensions]
- [runtools/main, jupyter_contrib_nbextensions]
- [select_keymap/main, jupyter_contrib_nbextensions]
- [snippets_menu/main, jupyter_contrib_nbextensions]
- [table_beautifier/main, jupyter_contrib_nbextensions]
- [jupyter-leaflet/extension, ipyleaflet]
