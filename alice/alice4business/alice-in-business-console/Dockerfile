FROM registry.yandex.net/paskills/alice-in-business-console-base:2 as builder
WORKDIR /build
ENV NODE_ENV=production


COPY ./files/package.json ./files/package-lock.json ./files/.npmrc ./
RUN npm ci --production=false
COPY ./files/ ./


ARG PASKILLS_URL_ROOT
ENV PASKILLS_URL_ROOT=${PASKILLS_URL_ROOT:-/b2b}
RUN npm run build
RUN mkdir /result
RUN cp -r build node_modules package.json package-lock.json /result/
RUN cd /result && npm prune --production && npm cache clean --force

FROM registry.yandex.net/toolbox/nodejs:10-xenial as runtime
WORKDIR /app
ENV NODE_ENV=production
CMD ["node", "build"]

COPY --from=builder /result /app
