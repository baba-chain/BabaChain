FROM phusion/baseimage:jammy-1.0.4
LABEL maintainer="BabaChain Developers <dev@babachain.org>"
LABEL description="Dockerised BabaChainCore, built from CI"

ARG USER_ID
ARG GROUP_ID

ENV HOME="/home/babachain"

# add user with specified (or default) user/group ids
ENV USER_ID="${USER_ID:-1000}"
ENV GROUP_ID="${GROUP_ID:-1000}"
RUN groupadd -g ${GROUP_ID} babachain && \
    useradd -u ${USER_ID} -g babachain -s /bin/bash -m -d /home/babachain babachain && \
    mkdir /home/babachain/.babachaincore && \
    chown ${USER_ID}:${GROUP_ID} -R /home/babachain

COPY bin/* /usr/local/bin/

RUN chmod a+x /usr/local/bin/*

USER babachain

VOLUME ["/home/babachain"]

EXPOSE 9998 9999 19998 19999

WORKDIR /home/babachain
