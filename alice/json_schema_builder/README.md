# JSON Schema based JSON builder generator

It uses a JSON Schema to generate builder classes that make generating and validating conforming JSON easier.

## How to generate the new version of div1 and div2

Use `./generate.sh`.

**Warning!** `--clean-directory` removes the output directory prior to generating code, so when calling `tool` by hand, make sure you don't accidentally delete your working copy or brick your machine :)

If `tool` finds something it doesn't understand in the schema, it will print the summary of what it didn't understand and won't generate anything at all.
You can then use `./tool/tool inspect` to figure out what happened and patch the tool.
