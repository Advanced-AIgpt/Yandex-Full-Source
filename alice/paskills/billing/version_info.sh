#!/bin/sh
count=`git rev-list HEAD | wc -l | tr -d '[:space:]'`
commit=`git show --abbrev-commit HEAD | grep '^commit' | sed -e 's/commit //'`
branch=`git rev-parse --abbrev-ref HEAD`
echo "$branch-1.0.$count-$commit"