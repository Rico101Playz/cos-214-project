FROM ubuntu:24.04 AS build
RUN apt-get update \
 && apt-get install -y --no-install-recommends g++ cmake ninja-build \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN rm -rf build \
 && cmake -S . -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release \
 && cmake --build /build \
 && /build/TIVIDY_Tests --gtest_brief=1

FROM ubuntu:24.04
RUN apt-get update \
 && apt-get install -y --no-install-recommends socat \
 && rm -rf /var/lib/apt/lists/*
COPY --from=build /build/TIVIDY_Server /build/TIVIDY_App /app/bin/
COPY web /app/web
COPY scenarios /app/scenarios-default
COPY docker/entrypoint.sh /app/entrypoint.sh
RUN sed -i "s/\r$//" /app/entrypoint.sh && chmod +x /app/entrypoint.sh
VOLUME /data
EXPOSE 8080 8081 8082
ENTRYPOINT ["/app/entrypoint.sh"]
