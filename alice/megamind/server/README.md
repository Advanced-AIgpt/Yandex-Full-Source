# MegaMind server

## Local run

The right way to run MegaMind is:
1. Build run script:
```
ya make --force-build-depends --checkout alice/megamind/scripts/run
```
Option ```--force-build-depends``` is necessary, option
```--checkout``` is needed on selective checkouts of Arcadia.
Feel free to add your favourite options if you need them, for example,
```--build=debug```, ```--sanitize=address``` or ```-j8```.
2. Run megamind via run script:
```
alice/megamind/scripts/run/run -p 17890 -c alice/megamind/configs/dev/megamind.pb.txt
```
This command runs megamind on 17890 port, and uses localhost
config. Feel free to change them if needed.
