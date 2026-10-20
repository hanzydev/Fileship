import 'dayjs/locale/de';
import 'dayjs/locale/fr';
import 'dayjs/locale/it';
import 'dayjs/locale/az';
import 'dayjs/locale/ja';
import 'dayjs/locale/ko';
import 'dayjs/locale/tr';
import 'dayjs/locale/zh';

import dayjs from 'dayjs';
import duration from 'dayjs/plugin/duration';
import localizedFormat from 'dayjs/plugin/localizedFormat';
import relativeTime from 'dayjs/plugin/relativeTime';

dayjs.extend(duration);
dayjs.extend(localizedFormat);
dayjs.extend(relativeTime);

export default defineNuxtPlugin(() => {
    if (import.meta.server) return;

    const { $i18n } = useNuxtApp();

    watch($i18n.locale, (locale) => dayjs.locale(locale), { immediate: true });
});
