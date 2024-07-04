# base image from which all other stages derive
ARG BASEIMAGE=alpine:latest

# derive from the appropriate base image depending on architecture
FROM --platform=linux/amd64 ${BASEIMAGE} AS base-amd64
FROM --platform=linux/arm64 ${BASEIMAGE} AS base-arm64
FROM --platform=linux/arm/v7 ${BASEIMAGE} AS base-arm
FROM --platform=linux/riscv64 riscv64/${BASEIMAGE} AS base-riscv64

FROM base-${TARGETARCH} as builder
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
FROM base-${TARGETARCH} as app
LABEL maintainer="lf-lang"
LABEL source="https://github.com/lf-lang/reactor-c/tree/main/core/federated/RTI"
COPY --from=builder /usr/local/bin/RTI /usr/local/bin/RTI

WORKDIR /lingua-franca

ENTRYPOINT ["/usr/local/bin/RTI"]
