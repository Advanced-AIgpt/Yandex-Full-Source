#!/bin/bash
cat regex-classes.txt | grep -Pv '^$|^#' | head -c -1 | tr '\n' '|'
