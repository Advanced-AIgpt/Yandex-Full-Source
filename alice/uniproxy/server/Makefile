INSTALL_PATH ?= /usr/bin
DOCKER_TEMPLATE ?= registry.yandex.net/voicetech/uniproxy
DELIVERY_TEMPLATE ?= registry.yandex.net/voicetech/unidelivery
DOCKER_VERSION ?= 3059.dddefd81.4
DOCKER_ARGS ?= --no-cache=false
DOCKER_BRANCH ?= testing

.PHONY: docker docker-cleanup docker-run docker-delivery

all:

clean:
	make -C src clean
	find . -name "*.pyc" -delete

docker: clean
	docker build $(DOCKER_ARGS) -t $(DOCKER_TEMPLATE):$(DOCKER_VERSION) .

docker-delivery: clean
	docker build $(DOCKER_ARGS) -f Dockerfile_Delivery -t $(DELIVERY_TEMPLATE):$(DOCKER_VERSION) .

docker-all: docker docker-delivery

docker-interactive:
	docker run -i --rm -e UNIPROXY_CUSTOM_ENVIRONMENT_TYPE=development --name testing -t $(DOCKER_TEMPLATE):$(DOCKER_VERSION) /bin/bash

docker-test-mem:
	docker run -i --rm -e UNIPROXY_CUSTOM_ENVIRONMENT_TYPE=development --name testing $(DOCKER_TEMPLATE):$(DOCKER_VERSION) \
  /bin/bash -c '/usr/bin/make -C /usr/lib/yandex/voice/uniproxy test_mem'

docker-cleanup:
	docker stop testing
	docker rm testing
	docker images | grep none | grep -oe "[0-9a-f]\{12\}" | sort -u | xargs docker rmi

push:
	docker push ${DOCKER_TEMPLATE}:${DOCKER_VERSION}

push-delivery:
	docker push ${DELIVERY_TEMPLATE}:${DOCKER_VERSION}

push-all: push push-delivery

