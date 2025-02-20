FROM node:22.12.0-alpine3.19 as builder

WORKDIR /fileship

COPY . .

RUN npm i -g corepack@latest
RUN corepack enable

RUN pnpm install --frozen-lockfile
RUN pnpm build

FROM node:22.12.0-alpine3.19 as runner

WORKDIR /fileship

RUN npm i -g corepack@latest
RUN corepack enable

COPY --from=builder /fileship/.output ./.output
COPY --from=builder /fileship/package.json ./package.json
COPY --from=builder /fileship/node_modules ./node_modules
COPY --from=builder /fileship/prisma ./prisma

EXPOSE 3000

CMD ["pnpm", "preview"]