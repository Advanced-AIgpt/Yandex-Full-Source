# Usage

`nlg` is a multicommand tool designed to manipulate NLG files.
All subcommands have the `--help` option.

`tool` is a multicommand tool designed to query the [compiled NLG](COMPILED_NLG.md) templates in the `config` directory.

## Tests

NLG's components do **not** recurse on their tests. You can either say `ya make -t ut` (or `ya make -t tests`), or say `ya make -t` in the top-level `nlg` directory.

## Render

You can render individual phrases and cards in `.nlg` files on the file system.
Use the `nlg render` command.

**WARNING**: this rendering scheme is considered legacy and should only be used for the purposes of developing and testing compiled NLG. Use `alice/nlg/tool` to test how templates will actually be rendered in Megamind.

**WARNING**: if your NLG template imports other templates, you must provide a value for `--import-dir` argument, otherwise the `file not found` error will be thrown. Import statements should be relative from the `--import-dir` value.

### Template:
```
{% phrase phrase_name %}
   Your phrase is "{{ context.a }}". Request id = {{ req_info.request_id }}
{% endphrase %}

{% macro render_text_from_form() -%}
  {% if form.road_event in ('error', 'error_no_route', 'error_no_turn') %}
      Ок, исправим.
  {% elif form.road_event in ('traffic_accidents', 'road_works', 'camera') %}
      Все сделано.
  {% else %}
      Готово.
  {% endif %}
{%- endmacro %}

{% card card_name %}
{
  "card": "{{ render_text_from_form() }}"
}
{% endphrase %}
```

Add the template to `alice/nlg/config/ya.make`, then recompile `alice/nlg/tool` and use its `render` command to query your template.

## C++ code generation

You can generate C++ source from `.nlg` files. Use the `nlg compile-cpp` command.
Arguments of the command:

- `--out-dir`           Directory that the output C++ library should be written to
- `--import-dir`        Directory that import statements should be relative from
- `--include-prefix`    Prefix for include direcitves in the generated source
- `--script`            NLG script to compile; you must specify all scripts that import each other for compilation to succeed

The source files will be generated in `out-dir/relative_path_to_template_from_import-dir`, `register.h` and `register.cpp` will be placed in `out-dir/include-prefix`.
You may not use `ext_import`, `ext_from`, `ext_nlgimport` for import of modules listed in `--script`, otherwise an ImportError will be thrown.

## NLG library

Use `COMPILE_NLG` macros in ya.make to collect all NLG templates. Paths to `.nlg` files must be relative from arcadia root.
There are two types of modules that can be imported in NLG template:
1. inner module: a module listed in `COMPILE_NLG`
2. outer module: a module that is not listed in `COMPILE_NLG`. An imported outer module must be a member of other NLG library. Dependencies must be specified in PEERDIR macros of ya.make.
For import of modules of other NLG libraries use commands `ext_import`, `ext_from`, `ext_nlgimport` in NLG templates. `import`, `from` and `nlgimport` can be employed only for import of modules listed in `COMPILE_NLG`. Usage of `import`, `from`, `nlgimport` for import of outer modules leads to `ImportError`. Usage of `ext_import`, `ext_from`, `ext_nlgimport` for import of inner modules creates a circular dependency in the graph, and so must be avoided, trying to do it will cause an error during ymake's import processing stage:
`Error[-WShowDirLoops]: in $B/alice/path_to_nlg_lib: the module will not be built due to deprecated loop`
`Configure error (use -k to proceed)`
