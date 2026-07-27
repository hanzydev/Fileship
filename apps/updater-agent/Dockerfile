FROM node:24-slim AS builder

WORKDIR /fileship

RUN apt-get update -y && apt-get install -y python3 make g++ \
    && ln -sf /usr/bin/python3 /usr/bin/python

COPY . .

RUN npm i -g corepack@latest
RUN corepack enable

RUN pnpm install
RUN pnpm --filter updater-agent... build

FROM node:24-slim AS runner

WORKDIR /fileship

RUN apt-get update -y && apt-get install -y docker.io docker-compose-v2 \
    && rm -rf /var/lib/apt/lists/*

RUN npm i -g corepack@latest
RUN corepack enable

COPY --from=builder /fileship/packages ./packages
COPY --from=builder /fileship/apps/updater-agent ./apps/updater-agent
COPY --from=builder /fileship/package.json ./package.json
COPY --from=builder /fileship/node_modules ./node_modules
COPY --from=builder /fileship/pnpm-workspace.yaml ./pnpm-workspace.yaml

EXPOSE 3001

CMD ["pnpm", "--filter", "updater-agent", "start"]