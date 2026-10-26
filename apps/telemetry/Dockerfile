FROM node:24-slim AS builder

WORKDIR /fileship

RUN apt-get update -y && apt-get install -y openssl python3 make g++ \
    && ln -sf /usr/bin/python3 /usr/bin/python

COPY . .

RUN npm i -g corepack@latest
RUN corepack enable

RUN pnpm install --frozen-lockfile --filter telemetry... --os=current --cpu=current --libc=current
RUN pnpm --filter telemetry... build

RUN npm install --prefix /prisma --omit=dev --omit=peer --no-audit --no-fund --no-package-lock \
    prisma@$(node -p "require('./apps/telemetry/node_modules/prisma/package.json').version") \
    dotenv@$(node -p "require('./apps/telemetry/node_modules/dotenv/package.json').version") \
    @vercel/nft@1.11.0 \
    && node scripts/prisma-cli.mjs /prisma /migrate

FROM node:24-slim AS runner

RUN apt-get update -y && apt-get install -y --no-install-recommends openssl ca-certificates \
    && rm -rf /var/lib/apt/lists/*

ENV CHECKPOINT_DISABLE=1

WORKDIR /fileship/apps/telemetry

COPY --from=builder /migrate/node_modules ./node_modules
COPY --from=builder /fileship/apps/telemetry/package.json /fileship/apps/telemetry/prisma.config.ts ./
COPY --from=builder /fileship/apps/telemetry/prisma ./prisma
COPY --from=builder /fileship/apps/telemetry/scripts/fixPrisma.js ./scripts/fixPrisma.js
COPY --from=builder /fileship/apps/telemetry/.output ./.output

EXPOSE 3000

CMD ["sh", "-c", "node node_modules/prisma/build/index.js migrate deploy && exec node -r ./scripts/fixPrisma.js .output/server/index.mjs"]
