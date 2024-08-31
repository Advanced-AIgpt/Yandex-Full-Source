FROM node:carbon
COPY . /front
WORKDIR /front

COPY package*.json ./
RUN npm install

COPY src /front/src
COPY public /front/public
RUN npm run build
RUN npm -g install serve

EXPOSE 8080
CMD ["serve", "-s", "build", "-p", "8080"]
