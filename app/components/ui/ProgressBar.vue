<template>
    <svg flex="~ items-center" w-full role="progressbar" width="100%" height="3">
        <rect
            height="3"
            ry="3"
            w="40%"
            max-w="1/2"
            fill-fs-accent
            animate="[indeterminate-1_2s_infinite]"
        ></rect>
        <rect
            height="3"
            rx="1.5"
            w="60%"
            max-w="1/2"
            fill-fs-accent
            animate="[indeterminate-2_2s_infinite]"
        ></rect>
    </svg>
</template>

<style>
@keyframes indeterminate-1 {
    0% {
        opacity: 1;
        transform: translate(-100%);
    }
    75% {
        opacity: 1;
        transform: translate(300%);
    }
    75.01% {
        opacity: 0;
    }
    to {
        opacity: 0;
        transform: translate(300%);
    }
}

@keyframes indeterminate-2 {
    0% {
        opacity: 0;
        transform: translate(-150%);
    }
    37.49% {
        opacity: 0;
    }
    37.5% {
        opacity: 1;
        transform: translate(-150%);
    }
    to {
        opacity: 1;
        transform: translate(166.66%);
    }
}
</style>
