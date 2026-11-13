FROM --platform=$BUILDPLATFORM ghcr.io/astral-sh/uv:0.13.8 AS uv

FROM --platform=$BUILDPLATFORM python:3.13-slim AS builder

ARG TARGETARCH

COPY --from=uv /uv /usr/local/bin/uv

WORKDIR /fileship/apps/ml

COPY apps/ml/pyproject.toml apps/ml/uv.lock ./
COPY apps/ml/src ./src

RUN platform=$(echo $TARGETARCH | sed 's/amd64/x86_64/;s/arm64/aarch64/')-manylinux_2_40 \
    && uv sync --frozen --no-dev --no-install-project --no-build --python-platform $platform \
    && uv sync --frozen --no-dev --no-editable --python-platform $platform

FROM python:3.13-slim AS runner

ENV PATH=/fileship/apps/ml/.venv/bin:$PATH
ENV FILESHIP_ML_HOST=0.0.0.0
ENV FILESHIP_ML_PORT=3003
ENV FILESHIP_ML_MODELS_DIR=/models

WORKDIR /fileship/apps/ml

COPY --from=builder /fileship/apps/ml/.venv ./.venv

EXPOSE 3003

CMD ["fileship-ml"]
