FROM node:22.12.0-slim AS builder

WORKDIR /fileship

RUN apt-get update -y && apt-get install -y openssl

COPY . .

RUN npm i -g corepack@latest
RUN corepack enable

RUN pnpm install
RUN pnpm build

FROM node:22.12.0-slim AS runner

WORKDIR /fileship

RUN apt-get update -y && apt-get install -y openssl
RUN npm i -g corepack@latest
RUN corepack enable

COPY --from=builder /fileship/packages ./packages
COPY --from=builder /fileship/apps ./apps
COPY --from=builder /fileship/package.json ./package.json
COPY --from=builder /fileship/node_modules ./node_modules
COPY --from=builder /fileship/pnpm-workspace.yaml ./pnpm-workspace.yaml

EXPOSE 3000

CMD ["pnpm", "--filter", "dashboard", "preview"]