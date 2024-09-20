<template>
    <div
        flex="~ col gap4 items-center justify-center"
        h48
        rounded-md
        bg-fs-overlay-2
        text-center
    >
        <div flex="~ gap4 items-center" text-slate300>
            <UiSpinner :size="32" />
            <h3>Loading...</h3>
        </div>
        <p text-slate400>Please wait while we fetch the data for you.</p>
    </div>
</template>
