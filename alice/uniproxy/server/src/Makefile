PYTEST_BIN ?= pytest

test_dev: compile_protobuf
	$(PYTEST_BIN) -s -v tests/vins_2.py
	$(PYTEST_BIN) -v tests/vins_bio.py

test_uniclient: compile_protobuf

test: test_ut
	(cd tests && $(PYTEST_BIN) -v pytest_uniclient.py)
	(cd tests && $(PYTEST_BIN) -v pytest_huniclient.py)
	$(PYTEST_BIN) -v tests/cit_test_storage.py
	$(PYTEST_BIN) -v tests/vins_2.py
	$(PYTEST_BIN) -v tests/vins_bio.py
	$(PYTEST_BIN) -v tests/vins_205.py
# need fix: use asr-gpu
#	$(PYTEST_BIN) -v tests/vins_call_to.py
#	$(PYTEST_BIN) -v tests/messaging.py

test_ut: check configure
	$(PYTEST_BIN) -v tests/unit_test*.py
	$(PYTEST_BIN) -v tests/mocked_uaas.py tests/mocked_experiments.py tests/mocked_asr.py

test_msg: check configure
	$(PYTEST_BIN) -v --log-level=DEBUG --log-cli-level=DEBUG tests/unit_test_messaging.py

test_mem: check configure
	$(PYTEST_BIN) -v --capture=no tests/memory_asr.py

notest: check compile_protobuf

pycheck:
	flake8 .

jsoncheck:
	for config in alice/uniproxy/library/settings/*.json; do echo $$config; cat $$config | json_pp -f json -t null; done

check: jsoncheck pycheck

pylint:
	python3 -m pylint --errors-only *.py backend/*.py monitoring/*.py processors/*.py tests/*.py

configure: compile_protobuf

compile_protobuf:
	protoc -I=. --python_out=. voicetech/library/proto_api/basic.proto
	protoc -I=. --python_out=. voicetech/library/proto_api/voiceproxy.proto
	protoc -I=. --python_out=. voicetech/library/proto_api/yaldi.proto
	protoc -I=. --python_out=. voicetech/library/proto_api/yabio.proto
	protoc -I=. --python_out=. voicetech/library/proto_api/ttsbackend.proto
	sed "s@^import \([^ ]*\)_pb2 as@import voicetech.library.proto_api.\1_pb2 as@" voicetech/library/proto_api/*_pb2.py -i
	sed 's@^import \([^ ]*\)_pb2$$@import voicetech.library.proto_api.\1_pb2 as \1_pb2@' voicetech/library/proto_api/*_pb2.py -i
	protoc --python_out=. -I. mssngr/router/lib/protos/message.proto mssngr/router/lib/protos/client.proto
	protoc --python_out=. -I. alice/uniproxy/library/protos/uniproxy.proto
	protoc --python_out=. -I. alice/uniproxy/library/protos/activation_storage.proto
	protoc --python_out=. -I. alice/uniproxy/library/protos/ttscache.proto

clean:
	rm -f voicetech/library/proto_api/*_pb2.py
	find . -name "*.pyc" -delete

.PHONY: all clean install
