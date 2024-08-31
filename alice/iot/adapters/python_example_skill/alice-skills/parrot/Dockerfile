FROM node:carbon
COPY . /parrot
COPY package*.json ./
WORKDIR /parrot
RUN npm install
EXPOSE 8002

CMD ["npm", "start"]