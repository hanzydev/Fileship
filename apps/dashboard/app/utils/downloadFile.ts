export const downloadFile = (url: string, fileName = '') => {
    const link = document.createElement('a');
    link.href = url;
    link.setAttribute('download', fileName);

    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
};
