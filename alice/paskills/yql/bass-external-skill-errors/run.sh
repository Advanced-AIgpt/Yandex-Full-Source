#!/usr/bin/env bash

set -e

yql -i report_1.sql -F library.sql@library.sql
yql -i report_2.sql -F library.sql@library.sql
yql -i report_3.sql -F library.sql@library.sql
yql -i report_4.sql -F library.sql@library.sql
yql -i report_5.sql -F library.sql@library.sql
yql -i report_6.sql -F library.sql@library.sql
yql -i report_7.sql -F library.sql@library.sql
