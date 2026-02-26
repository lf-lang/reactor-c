ARG BASEIMAGE=alpine:latest 
FROM ${BASEIMAGE} AS builder
COPY . /lingua-franca
WORKDIR /lingua-franca/core/federated/RTI
RUN set -ex && apk add --no-cache gcc musl-dev cmake make git && \
    mkdir container && \
    cd container && \
    cmake ../ && \
    make && \
    make install

WORKDIR /lingua-franca

# application stage
FROM ${BASEIMAGE} AS app
LABEL maintainer="lf-lang"
LABEL source="https://github.com/lf-lang/reactor-c/tree/main/core/federated/RTI"
COPY --from=builder /usr/local/bin/RTI /usr/local/bin/RTI

WORKDIR /lingua-franca

ENTRYPOINT ["/usr/local/bin/RTI"]
