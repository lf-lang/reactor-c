ARG BASEIMAGE=alpine:latest 
FROM ${BASEIMAGE} as builder
COPY . /lingua-franca
WORKDIR /lingua-franca/core/federated/RTI
RUN apk add --no-cache \
        gcc musl-dev cmake make \
        libressl-dev
RUN mkdir -p build
WORKDIR /lingua-franca/core/federated/RTI/build
RUN cmake -DAUTH=on ..
RUN make
RUN make install

WORKDIR /lingua-franca

# application stage
FROM ${BASEIMAGE} as app
LABEL maintainer="lf-lang"
LABEL source="https://github.com/lf-lang/reactor-c/tree/main/core/federated/RTI"
COPY --from=builder /usr/local/bin/RTI /usr/local/bin/RTI

WORKDIR /lingua-franca

ENTRYPOINT ["/usr/local/bin/RTI"]
