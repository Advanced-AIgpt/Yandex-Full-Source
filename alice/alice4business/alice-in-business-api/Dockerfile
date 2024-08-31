FROM registry.yandex.net/paskills/alice-in-business-api-base:1 as builder
WORKDIR /build
ENV NODE_ENV=production

COPY ./files/package.json ./files/package-lock.json ./files/.npmrc ./
RUN npm ci --production=false
COPY ./files/ ./

RUN npm run build
RUN cp -r ./build /result
RUN cp -r node_modules package.json package-lock.json .sequelizerc /result/
RUN cd /result && npm prune --production && npm cache clean --force

FROM registry.yandex.net/paskills/alice-in-business-api-base:1 as runtime
WORKDIR /app
ENV NODE_ENV=production
CMD ["node", "server"]

COPY --from=builder /result /app
