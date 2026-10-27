FROM --platform=$BUILDPLATFORM node:24-slim AS builder

ARG TARGETARCH

WORKDIR /fileship

RUN apt-get update -y && apt-get install -y python3 make g++ \
    && ln -sf /usr/bin/python3 /usr/bin/python

COPY . .

RUN npm i -g corepack@latest
RUN corepack enable

RUN pnpm install --frozen-lockfile --filter updater-agent... --os=current --cpu=current --libc=current
RUN pnpm --filter updater-agent... build
RUN pnpm --filter updater-agent deploy --prod --legacy /deploy
RUN node scripts/retarget-natives.mjs /deploy $TARGETARCH

FROM node:24-slim AS runner

COPY --from=docker:cli /usr/local/bin/docker /usr/local/bin/docker
COPY --from=docker/compose-bin:latest /docker-compose /usr/local/libexec/docker/cli-plugins/docker-compose

RUN ln -s /usr/local/libexec/docker/cli-plugins/docker-compose /usr/local/bin/docker-compose

WORKDIR /fileship/apps/updater-agent

COPY --from=builder /fileship/package.json /fileship/package.json
COPY --from=builder /deploy ./

EXPOSE 3001

CMD ["node", "."]
