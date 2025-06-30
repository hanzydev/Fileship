<template>
    <div
        flex="~ col gap4 items-center justify-center"
        h48
        rounded-xl
        bg-fs-overlay-2
        text-center
        ring="1 fs-overlay-4"
    >
        <div flex="~ gap4 items-center" text-fs-muted-2>
            <UiSpinner :size="32" />
            <h3>Loading...</h3>
        </div>
        <p text-fs-muted-2>Please wait while we fetch the data for you.</p>
    </div>
</template>
