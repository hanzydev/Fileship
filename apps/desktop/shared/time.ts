const TIME_UNITS: [Intl.RelativeTimeFormatUnit, number][] = [
    ['year', 31_536_000],
    ['month', 2_592_000],
    ['week', 604_800],
    ['day', 86_400],
    ['hour', 3600],
    ['minute', 60],
];

export const relativeTime = (locale: string, value: number, now: number, justNow: string) => {
    const seconds = (value - now) / 1000;
    const unit = TIME_UNITS.find(([, size]) => Math.abs(seconds) >= size);

    if (!unit) return justNow;

    return new Intl.RelativeTimeFormat(locale, { numeric: 'auto' }).format(
        Math.round(seconds / unit[1]),
        unit[0],
    );
};
