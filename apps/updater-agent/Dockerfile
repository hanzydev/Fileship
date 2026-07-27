FROM node:24-alpine AS builder

WORKDIR /fileship

RUN apk add --no-cache python3 make g++

COPY . .

RUN npm i -g corepack@latest
RUN corepack enable

RUN pnpm install
RUN pnpm --filter updater-agent... build

FROM node:24-alpine AS runner

WORKDIR /fileship

RUN apk add --no-cache docker-cli docker-cli-compose
RUN npm i -g corepack@latest
RUN corepack enable

COPY --from=builder /fileship/packages ./packages
COPY --from=builder /fileship/apps/updater-agent ./apps/updater-agent
COPY --from=builder /fileship/package.json ./package.json
COPY --from=builder /fileship/node_modules ./node_modules
COPY --from=builder /fileship/pnpm-workspace.yaml ./pnpm-workspace.yaml

EXPOSE 3001

CMD ["pnpm", "--filter", "updater-agent", "start"]