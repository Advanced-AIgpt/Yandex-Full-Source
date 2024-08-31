FROM registry.yandex.net/toolbox/nodejs:14.15.1-bionic-minimal

RUN apt-get update && apt-get -q -y install build-essential && apt-get clean

WORKDIR /app

COPY .npmrc package.json ./
RUN npm i
COPY . ./
RUN npm run build

CMD ["node", "build"]
