FROM frolvlad/alpine-gxx

RUN apk update
RUN apk add make
RUN apk add cmake

WORKDIR /usr/jarl
