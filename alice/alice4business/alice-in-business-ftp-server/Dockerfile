FROM registry.yandex.net/toolbox/nodejs:14-bionic

RUN mkdir "app"
WORKDIR /app

COPY ./files/package.json ./files/package-lock.json ./files/.npmrc ./
RUN npm ci --production=false
COPY ./files/ ./

RUN npm run build
RUN npm prune --production && npm cache clean --force
RUN mkdir "./build/uploads"
RUN mkdir "./build/certs"

COPY ./files/certs ./build/certs/

CMD ["node", "build/server.js"]

EXPOSE 21
EXPOSE 65500-65504
EXPOSE 80

