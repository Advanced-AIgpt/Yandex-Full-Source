# Python wrapper for easy TNlgSearch use (a.k.a vins general conversation neural network "boltalka")

## Building for python 2

1. `ya make -r -DUSE_ARCADIA_PYTHON=no -DOS_SDK=local`. This command will produce `nlgsearch.so` which you can import from python.
2. Optionally, add `export PYTHONPATH="/place/home/persiyanov/arcadia/alice/boltalka/py_libs/nlgsearch_simple:$PYTHONPATH"` to be able to do `import nlgsearch` everywhere.

## Building for python 3

1. `ya make -r -DUSE_ARCADIA_PYTHON=no -DOS_SDK=local -DPYTHON_INCLUDE="-I/usr/include/python3.4"`. This command will produce `nlgsearch.so` which you can import from python.
2. Optionally, add `export PYTHONPATH="/place/home/persiyanov/arcadia/alice/boltalka/py_libs/nlgsearch_simple:$PYTHONPATH"` to be able to do `import nlgsearch` everywhere.
