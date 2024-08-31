The structure of the directory is following:
* builder/
  A tool used to prepare all necessary regions data from the
  GEODATA5BIN_STABLE.  Note that you need to manually upload result
  file from the tool to the sandbox and update resource id in the
  ya.make file.

* demo/
  A command line tool that can be used to play with search over localities.

* research/
  Tool prints pairs of nested regions with same names.

* prepare-regions.sh
  This script launches builder tool.

* quality.py
  This script can be used to estimate search accuracy. As the user of
  the library is usually interested in the top locality matched by a
  query, accuracy is measured here.

* queries.csv
  A small dataset consisting of queries that can't be handled by geoaddr.

Core library is located in quality/functionality/cards_service/libs/smallgeo.

For details about libgeobase, see
https://doc.yandex-team.ru/lib/libgeobase5/.
