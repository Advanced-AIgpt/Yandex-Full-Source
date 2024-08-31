ulimit -c 1000000000
#rm cuttlefish.evlog ; gdb --args ./cuttlefish -p 4000 --loop-threads 3
rm -f cuttlefish.evlog cuttlefish.rtlog ; ./cuttlefish run -V server.grpc.port=4001 -V  server.http.port=4000 -V megamind.default_url=http://vins.alice.yandex.net
#rm cuttlefish.evlog ; ./cuttlefish run -V server.grpc.port=40011 -V  server.http.port=40010 -V megamind.default_url=http://megamind-hamster.hamster.alice.yandex.net
#rm cuttlefish.evlog ; ./cuttlefish-trunk run -V server.grpc.port=40011 -V  server.http.port=4000 -V megamind.default_url=http://megamind-hamster.hamster.alice.yandex.net
#rm cuttlefish.evlog ; ./cuttlefish-d -p 4000 --loop-threads 10
#rm cuttlefish.evlog ; valgrind --log-file=valgrind.log ./cuttlefish -p 4000 --loop-threads 3
#rm cuttlefish.evlog ; gdb --args ./cuttlefish-69 -p 4000 --loop-threads 10
# No newline at end of file
#rm cuttlefish.evlog ; gdb --args ./cuttlefish-69 -p 4000 --loop-threads 10
# for srcrwr
#rm cuttlefish.evlog ; ./cuttlefish -p 40010 --loop-threads 10
