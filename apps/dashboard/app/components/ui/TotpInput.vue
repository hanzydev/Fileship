<template>
    <div space-y-1>
        <UiLabel :for="id" :error required>TOTP Code</UiLabel>
        <div :id grid="~ cols-6 gap2 sm:gap4">
            <input
                v-for="digit in 6"
                :key="digit"
                :ref="(el) => (totpInputs[digit - 1] = el as never)"
                h12
                w10
                rounded-lg
                bg-fs-overlay-3
                text-center
                text-xl
                outline-none
                ring="1 fs-overlay-4"
                sm="h-14 w12 !text-2xl"
                font-semibold="!"
                motion-safe:transition-shadow
                type="number"
                :disabled
                :class="[inputClass, disabled ? 'cursor-not-allowed' : 'focus:ring-fs-accent']"
                @input="(event) => handleInput(event, digit - 1)"
                @keydown.delete="(event) => handleBackspace(event, digit - 1)"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
const totpInputs = ref<HTMLInputElement[]>([]);

defineProps<{
    error?: string;
    disabled?: boolean;
    inputClass?: unknown;
}>();

const emit = defineEmits<{
    got: [totp: string];
}>();

const value = defineModel<string>({ required: false, default: '' });

const id = useId();

const handleInput = (e: Event, index: number) => {
    const input = e.target as HTMLInputElement;
    const val = input.value.replaceAll(' ', '');

    const first = val[0];
    const rest = val.slice(1);

    input.value = first ?? '';

    const lastInputBox = index === totpInputs.value.length - 1;
    const didInsertContent = first !== undefined;

    if (didInsertContent && lastInputBox) {
        const _value = totpInputs.value.map((el) => el.value).join('');
        value.value = _value;
        emit('got', _value);
    }

    if (didInsertContent && !lastInputBox) {
        totpInputs.value[index + 1]!.focus();

        if (rest.length > 1) {
            for (let inputIndex = 0; inputIndex < totpInputs.value.length; inputIndex++) {
                const char = (first + rest)[inputIndex];
                if (!char) break;

                totpInputs.value[inputIndex]!.focus();
                totpInputs.value[inputIndex]!.value = char;

                if (inputIndex === 5) {
                    const _value = totpInputs.value.map((el) => el.value).join('');
                    value.value = _value;
                    emit('got', _value);
                }
            }
        } else {
            totpInputs.value[index + 1]!.value = rest;
        }
    }
};

const handleBackspace = (e: any, index: number) => {
    if (e.target.value === '') {
        totpInputs.value[Math.max(0, index - 1)]!.focus();
    }
};
</script>
