interface FileSettings {
    fileNameType: 'Random' | 'UUID' | 'Original';
    maxViews: number;
    password: string | null;
    expiration: number | null;
    compression: number;
    folder: string | null;
}

export const uploadFile = async (
    file: File,
    settings: FileSettings = {
        fileNameType: 'Random',
        maxViews: 0,
        password: null,
        expiration: null,
        compression: 0,
        folder: null,
    },
) => {
    let {
        public: { fileChunkSize },
    } = useRuntimeConfig();
    const uploadingFiles = useUploadingFiles();

    fileChunkSize = (+fileChunkSize || 25) * 1024 * 1024;

    const chunks = Math.ceil(file.size / fileChunkSize);
    const uploadingFile = uploadingFiles.value.find((f) => f.name === file.name);

    if (!uploadingFile) return;

    if (!uploadingFile.status) {
        uploadingFile.status = reactive({
            started: false,
            progress: {
                speed: 0,
                percent: 0,
                eta: 0,
            },
            error: null,
        });
    }

    uploadingFile.status!.started = true;

    const startedAt = Date.now();
    let totalLoaded = 0;

    for (let i = 0; i < chunks; i++) {
        const start = i * fileChunkSize;
        const end = Math.min(file.size, start + fileChunkSize);
        const chunk = file.slice(start, end);

        const formData = new FormData();

        formData.append('file', new Blob([chunk], { type: file.type }), file.name);
        formData.append('currentChunk', (i + 1).toString());
        formData.append('totalChunks', chunks.toString());
        formData.append('fileNameType', settings.fileNameType);
        formData.append('maxViews', settings.maxViews.toString());
        formData.append('compression', settings.compression.toString());

        if (settings.password) formData.append('password', settings.password);
        if (settings.expiration) formData.append('expiration', settings.expiration.toString());
        if (settings.folder) formData.append('folderId', settings.folder);

        const res = await new Promise<boolean>((resolve) => {
            const req = new XMLHttpRequest();

            let lastLoaded = 0;

            req.upload.addEventListener('progress', (e) => {
                if (e.lengthComputable) {
                    const sent = e.loaded - lastLoaded;
                    lastLoaded = e.loaded;
                    totalLoaded += sent;

                    const speed = totalLoaded / ((Date.now() - startedAt) / 1000);
                    const percent = Math.round((totalLoaded / file.size) * 100);
                    const eta = Math.round((file.size - totalLoaded) / speed);

                    uploadingFile.status!.progress = {
                        speed,
                        percent,
                        eta,
                    };
                }
            });

            req.addEventListener('load', () => {
                if (req.responseText.length) {
                    const res = JSON.parse(req.responseText);
                    if (res.error) {
                        uploadingFile.status!.error = res.message;
                        resolve(false);
                        return;
                    }
                }

                uploadingFile.status!.error = null;
                resolve(true);
            });

            req.open('POST', '/api/files');
            req.send(formData);
        });

        if (!res) return false;
    }

    return true;
};
