import { randomBytes } from 'node:crypto';

export const generateUploadToken = async (userId: string) => {
    const token = `fs_${randomBytes(30).toString('base64url')}`;

    await prisma.user.update({
        where: { id: userId },
        data: { uploadTokenHash: hashToken(token), uploadTokenEncrypted: await encrypt(token) },
    });

    return token;
};
