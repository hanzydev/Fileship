FROM --platform=$BUILDPLATFORM node:24-slim AS builder

ARG TARGETARCH

WORKDIR /fileship

RUN apt-get update -y && apt-get install -y openssl python3 make g++ \
    && ln -sf /usr/bin/python3 /usr/bin/python

COPY . .

RUN npm i -g corepack@latest
RUN corepack enable

RUN pnpm install --frozen-lockfile --filter dashboard... --os=current --libc=current \
    --cpu=current --cpu=$(echo $TARGETARCH | sed s/amd64/x64/)
RUN pnpm --filter dashboard... build
RUN node scripts/retarget-natives.mjs apps/dashboard/.output $TARGETARCH

RUN export PRISMA_CLI_BINARY_TARGETS=$([ $TARGETARCH = arm64 ] && echo linux-arm64 || echo debian)-openssl-3.0.x \
    && npm install --prefix /prisma --omit=dev --omit=peer --no-audit --no-fund --no-package-lock \
    prisma@$(node -p "require('./apps/dashboard/node_modules/prisma/package.json').version") \
    dotenv@$(node -p "require('./apps/dashboard/node_modules/dotenv/package.json').version") \
    @vercel/nft@1.11.0 \
    && node scripts/prisma-cli.mjs /prisma /migrate \
    && node scripts/retarget-natives.mjs /migrate $TARGETARCH

FROM node:24-slim AS runner

RUN apt-get update -y && apt-get install -y --no-install-recommends openssl ca-certificates \
    && rm -rf /var/lib/apt/lists/*

ENV CHECKPOINT_DISABLE=1

WORKDIR /fileship/apps/dashboard

COPY --from=builder /migrate/node_modules ./node_modules
COPY --from=builder /fileship/apps/dashboard/package.json /fileship/apps/dashboard/prisma.config.ts ./
COPY --from=builder /fileship/apps/dashboard/prisma ./prisma
COPY --from=builder /fileship/apps/dashboard/scripts/fixPrisma.js ./scripts/fixPrisma.js
COPY --from=builder /fileship/apps/dashboard/.output ./.output

EXPOSE 3000

CMD ["sh", "-c", "node node_modules/prisma/build/index.js migrate deploy && exec node -r ./scripts/fixPrisma.js .output/server/index.mjs"]
