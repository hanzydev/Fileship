FROM node:24-slim AS builder

WORKDIR /fileship

RUN apt-get update -y && apt-get install -y openssl python3 make g++ \
    && ln -sf /usr/bin/python3 /usr/bin/python

COPY . .

RUN npm i -g corepack@latest
RUN corepack enable

RUN pnpm install --frozen-lockfile --filter telemetry...
RUN pnpm --filter telemetry... build

FROM node:24-slim AS runner

WORKDIR /fileship

RUN apt-get update -y && apt-get install -y openssl
RUN npm i -g corepack@latest
RUN corepack enable

COPY --from=builder /fileship/packages ./packages
COPY --from=builder /fileship/apps/telemetry ./apps/telemetry
COPY --from=builder /fileship/package.json ./package.json
COPY --from=builder /fileship/node_modules ./node_modules
COPY --from=builder /fileship/pnpm-workspace.yaml ./pnpm-workspace.yaml

EXPOSE 3000

WORKDIR /fileship/apps/telemetry

CMD ["sh", "-c", "pnpm db:deploy && exec node -r ./scripts/fixPrisma.js .output/server/index.mjs"]
