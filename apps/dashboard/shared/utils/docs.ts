import { version } from '../../../../package.json';

export const docsUrl = (path: string) =>
    `https://fileship.app/docs/${version.includes('-') ? 'experimental/' : ''}${path}`;
