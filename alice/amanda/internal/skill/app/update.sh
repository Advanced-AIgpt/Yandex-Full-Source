#!/bin/bash

APP=$( dirname -- "$0"; )

ya make $APP/generator
$APP/generator/generate
ya tool go fmt $APP/presets.go
