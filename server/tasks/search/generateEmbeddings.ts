import consola from 'consola';
import dayjs from 'dayjs';
import { extname, join } from 'pathe';

import { Prisma } from '@prisma/client';

export default defineTask({
    meta: {
        name: 'search:generateEmbeddings',
        description: 'Generates embeddings for files using OpenAI/CLIP',
    },
    async run() {
        const clip = await getClipInstance();

        const files = await prisma.file.findMany({
            where: {
                embedding: {
                    equals: Prisma.AnyNull,
                },
            },
            select: {
                id: true,
                fileName: true,
            },
        });

        const filteredFiles = files.filter((file) =>
            IMAGE_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extname(file.fileName)),
        );

        if (filteredFiles.length) {
            consola.info(
                `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Processing ${files.length} files for embedding...`,
            );
        }

        const embeddings = await Promise.all(
            filteredFiles.map(async (file) => {
                const filePath = join(dataDirectory, 'uploads', file.fileName);
                return { id: file.id, embedding: await clip.createImageEmbedding(filePath) };
            }),
        );

        await prisma.$transaction(
            embeddings.map(({ id, embedding }) =>
                prisma.file.update({
                    where: { id },
                    data: { embedding },
                }),
            ),
        );

        if (embeddings.length) {
            consola.success(
                `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Processed ${embeddings.length} files for embedding.`,
            );
        }

        return { result: 'success' };
    },
});
