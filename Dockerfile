FROM frolvlad/alpine-gxx

RUN apk update
RUN apk add make cmake re2c

WORKDIR /usr/jarl
