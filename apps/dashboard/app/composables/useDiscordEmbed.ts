export const useDiscordEmbed = (embed: () => string | null | undefined) =>
    useHead(() => ({
        script: embed()
            ? [{ id: 'discord:component-embed', type: 'application/json', innerHTML: embed()! }]
            : [],
    }));
