#!/bin/bash

docker run --rm -it  --platform linux/amd64  -v $(pwd):/app -w /app \
  debian:bookworm-slim \
  bash build_docker.sh
