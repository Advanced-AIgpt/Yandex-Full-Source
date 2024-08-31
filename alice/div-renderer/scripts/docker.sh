#!/bin/bash

IMAGE=paskills/div-renderer:$TAG
REGISTRY=registry.yandex.net

npm run build
npm prune --production
docker login -u $DOCKER_USERNAME -p $DOCKER_OAUTH_TOKEN $REGISTRY
docker build -t $IMAGE .
docker tag $IMAGE $REGISTRY/$IMAGE
docker push $REGISTRY/$IMAGE