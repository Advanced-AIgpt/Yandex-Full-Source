#!/bin/bash
APOS="'"
REGEX='s/ ́/ /g; s/­//g; s/‚/,/g; s/ё/е/g; s/Ё/E/g; s/«|»|“|”|„|``/"/g; s/’|‘|`/'$APOS'/g; s/…/.../g; s/\[[^]]*\]//g; s/\{[^}]*\}//g'
sed -re "$REGEX"
