<template>
    <div space-y-1>
        <UiLabel :for="id" :error required>TOTP Code</UiLabel>
        <div :id class="grid grid-cols-6 gap4">
            <input
                v-for="digit in 6"
                :key="digit"
                :ref="(el) => (totpInputs[digit - 1] = el as never)"
                h12
                w10
                rounded-md
                bg-fs-overlay-2
                text-center
                text-xl
                outline-none
                sm="h-14 w12 !text-2xl"
                font-semibold="!"
                focus:ring-1
                focus:ring-fs-accent
                motion-safe:transition-shadow
                type="number"
                :disabled
                :class="inputClass"
                @input="(event) => handleInput(event, digit - 1)"
                @keydown.delete="(event) => handleBackspace(event, digit - 1)"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
const totpInputs = reactive<HTMLInputElement[]>([]);

defineProps<{
    error?: string;
    disabled?: boolean;
    inputClass?: unknown;
}>();

const emit = defineEmits<{
    got: [totp: string];
}>();

const id = useId();

const handleInput = (e: Event, index: number) => {
    const input = e.target as HTMLInputElement;
    const val = input.value.replaceAll(' ', '');

    const first = val[0];
    const rest = val.slice(1);

    input.value = first ?? '';

    const lastInputBox = index === totpInputs.length - 1;
    const didInsertContent = first !== undefined;

    if (didInsertContent && lastInputBox) {
        emit('got', totpInputs.map((el) => el.value).join(''));
    }

    if (didInsertContent && !lastInputBox) {
        totpInputs[index + 1]!.focus();

        if (rest.length > 1) {
            for (
                let inputIndex = 0;
                inputIndex < totpInputs.length;
                inputIndex++
            ) {
                const char = (first + rest)[inputIndex];
                if (!char) break;

                totpInputs[inputIndex]!.focus();
                totpInputs[inputIndex]!.value = char;

                if (inputIndex === 5) {
                    emit('got', totpInputs.map((el) => el.value).join(''));
                }
            }
        } else {
            totpInputs[index + 1]!.value = rest;
        }
    }
};

const handleBackspace = (e: any, index: number) => {
    if (e.target.value === '') totpInputs[Math.max(0, index - 1)]!.focus();
};

onMounted(() => totpInputs[0]?.focus());
</script>
