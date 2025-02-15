# started from https://cloud.google.com/run/docs/quickstarts/build-and-deploy/deploy-c-plus-plus-service

FROM docker.io/ubuntu:24.04 AS build

RUN apt update \
    && apt -y -q install gcc time libcurl4-openssl-dev zlib1g-dev \
    && apt clean

# Copy the source code to /v/source and compile it.
COPY . /src
WORKDIR /src

RUN ./jcc build

# Create the final deployment image, using `scratch` (the empty Docker image)
# as the starting point. Effectively we create an image that only contains
# our program.
FROM docker.io/ubuntu:24.04 AS app
WORKDIR /

RUN apt update \
    && apt -y -q install libcurl4t64 zlib1g \
    && apt clean

COPY --from=build /src/AppDir /AppDir
COPY --from=build /src/main /main

RUN useradd -m -U appuser && mkdir /data && chown -R appuser:appuser /main /AppDir /data

USER appuser

VOLUME /data

EXPOSE 8080

ENV APPDIR=/AppDir

CMD ["/main", "/data" ]

