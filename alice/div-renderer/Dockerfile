FROM registry.yandex.net/toolbox/nodejs:16.13.2-bionic-minimal

RUN apt-get update && apt-get -q -y install build-essential && apt-get clean

WORKDIR /app
COPY package.json /app
COPY package-lock.json /app
RUN npm ci

COPY . /app
ENV NODE_OPTIONS="--max_old_space_size=4096"
RUN npm run build
RUN npm prune --production

CMD npm start
