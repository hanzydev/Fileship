<template>
    <Head>
        <Title>{{ $t('instance.title') }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader :title="$t('instance.title')" :description="$t('instance.description')" />

        <div class="grid items-start gap-4 lg:grid-cols-2">
            <template v-if="!data">
                <div v-for="i in 2" :key="i" class="flex flex-col gap-4">
                    <UiCard v-for="j in 3" :key="j">
                        <UiCardHeader>
                            <UiSkeleton class="h-5 w-32" />
                            <UiSkeleton class="h-4 w-64" />
                        </UiCardHeader>
                        <UiCardContent class="flex flex-col gap-4">
                            <UiSkeleton v-for="k in 2" :key="k" class="h-9 w-full" />
                        </UiCardContent>
                    </UiCard>
                </div>
            </template>

            <template v-else>
                <div
                    v-for="(column, columnIndex) in columns"
                    :key="columnIndex"
                    class="flex flex-col gap-4"
                >
                    <UiCard v-for="section in column" :key="section.id">
                        <form class="contents" @submit.prevent="handleSave(section)">
                            <UiCardHeader>
                                <UiCardTitle>
                                    {{ $t(`instance.sections.${section.id}.title`) }}
                                </UiCardTitle>
                                <UiCardDescription>
                                    {{ $t(`instance.sections.${section.id}.description`) }}
                                </UiCardDescription>
                            </UiCardHeader>
                            <UiCardContent>
                                <UiFieldGroup class="grid gap-x-4 sm:grid-cols-2">
                                    <template
                                        v-for="(field, index) in section.fields"
                                        :key="field.key"
                                    >
                                        <UiFieldSeparator
                                            v-if="index && field.type === 'switch'"
                                            class="sm:col-span-2"
                                        />

                                        <UiField
                                            v-if="field.type === 'switch'"
                                            orientation="horizontal"
                                            class="has-[>[data-slot=field-content]]:items-center sm:col-span-2"
                                        >
                                            <UiFieldContent>
                                                <UiFieldLabel :for="field.key">
                                                    {{ $t(field.label) }}
                                                </UiFieldLabel>
                                                <UiFieldDescription>
                                                    {{ $t(field.description!) }}
                                                </UiFieldDescription>
                                                <UiFieldDescription v-if="isOverridden(field.key)">
                                                    {{
                                                        $t('instance.default', {
                                                            value: formatDefault(field),
                                                        })
                                                    }}
                                                </UiFieldDescription>
                                            </UiFieldContent>
                                            <UiButton
                                                v-if="isOverridden(field.key)"
                                                type="button"
                                                variant="ghost"
                                                size="icon-sm"
                                                :aria-label="$t('instance.resetToDefault')"
                                                :disabled="resettingKey === field.key"
                                                @click="handleReset(field.key)"
                                            >
                                                <UiSpinner v-if="resettingKey === field.key" />
                                                <IRotateCcw v-else />
                                            </UiButton>
                                            <UiSwitch
                                                :id="field.key"
                                                v-model="form[field.key] as boolean"
                                            />
                                        </UiField>

                                        <UiField v-else :class="!field.half && 'sm:col-span-2'">
                                            <div class="flex items-center justify-between gap-2">
                                                <UiFieldLabel :for="field.key">
                                                    {{ $t(field.label) }}
                                                </UiFieldLabel>
                                                <UiTooltip v-if="isOverridden(field.key)">
                                                    <UiTooltipTrigger as-child>
                                                        <UiButton
                                                            type="button"
                                                            variant="ghost"
                                                            size="icon-xs"
                                                            :aria-label="
                                                                $t('instance.resetToDefault')
                                                            "
                                                            :disabled="resettingKey === field.key"
                                                            @click="handleReset(field.key)"
                                                        >
                                                            <UiSpinner
                                                                v-if="resettingKey === field.key"
                                                            />
                                                            <IRotateCcw v-else />
                                                        </UiButton>
                                                    </UiTooltipTrigger>
                                                    <UiTooltipContent>
                                                        {{
                                                            $t('instance.resetTo', {
                                                                value: formatDefault(field),
                                                            })
                                                        }}
                                                    </UiTooltipContent>
                                                </UiTooltip>
                                            </div>

                                            <UiInput
                                                v-if="field.type === 'text'"
                                                :id="field.key"
                                                v-model="form[field.key] as string"
                                                :placeholder="field.placeholder"
                                            />

                                            <PasswordInput
                                                v-else-if="field.type === 'secret'"
                                                :id="field.key"
                                                v-model="secrets[field.key as SecretSettingKey]"
                                                autocomplete="off"
                                                :placeholder="
                                                    data.secretsSet[field.key as SecretSettingKey]
                                                        ? $t('instance.leaveEmptyToKeep')
                                                        : ''
                                                "
                                            />

                                            <UiNumberField
                                                v-else-if="field.type === 'number'"
                                                :id="field.key"
                                                v-model="form[field.key] as number"
                                                :min="field.min"
                                                :max="field.max"
                                                :format-options="field.formatOptions"
                                            >
                                                <UiNumberFieldContent>
                                                    <UiNumberFieldDecrement />
                                                    <UiNumberFieldInput />
                                                    <UiNumberFieldIncrement />
                                                </UiNumberFieldContent>
                                            </UiNumberField>

                                            <UiSelect
                                                v-else-if="field.type === 'select'"
                                                :model-value="String(form[field.key])"
                                                @update:model-value="
                                                    handleSelect(field.key, $event as string)
                                                "
                                            >
                                                <UiSelectTrigger :id="field.key" class="w-full">
                                                    <UiSelectValue />
                                                </UiSelectTrigger>
                                                <UiSelectContent>
                                                    <UiSelectItem
                                                        v-for="option in field.options"
                                                        :key="option.value"
                                                        :value="option.value"
                                                    >
                                                        {{ $t(option.label) }}
                                                    </UiSelectItem>
                                                </UiSelectContent>
                                            </UiSelect>

                                            <div
                                                v-else-if="field.type === 'permissions'"
                                                class="flex flex-col gap-3"
                                            >
                                                <UiField
                                                    v-for="permission in registrablePermissions"
                                                    :key="permission.value"
                                                    orientation="horizontal"
                                                >
                                                    <UiCheckbox
                                                        :id="`permission-${permission.value}`"
                                                        :model-value="
                                                            form.registrationPermissions.includes(
                                                                permission.value,
                                                            )
                                                        "
                                                        @update:model-value="
                                                            handleTogglePermission(permission.value)
                                                        "
                                                    />
                                                    <UiFieldLabel
                                                        :for="`permission-${permission.value}`"
                                                    >
                                                        {{ $t(permission.label) }}
                                                    </UiFieldLabel>
                                                </UiField>
                                            </div>

                                            <UiFieldDescription v-if="field.description">
                                                {{ $t(field.description!) }}
                                            </UiFieldDescription>
                                            <UiFieldError
                                                v-if="formErrors?.[field.key]?._errors?.length"
                                                :errors="formErrors[field.key]._errors"
                                            />
                                        </UiField>
                                    </template>

                                    <template v-if="section.id === 'email'">
                                        <UiFieldSeparator class="sm:col-span-2" />
                                        <UiField class="sm:col-span-2">
                                            <UiFieldLabel for="test-email">
                                                {{ $t('instance.email.test') }}
                                            </UiFieldLabel>
                                            <div class="flex gap-2">
                                                <UiInput
                                                    id="test-email"
                                                    v-model="testEmail"
                                                    type="email"
                                                    autocomplete="email"
                                                    @keydown.enter.prevent="handleSendTest"
                                                />
                                                <UiButton
                                                    type="button"
                                                    variant="outline"
                                                    :disabled="!testEmail || isSendingTest"
                                                    @click="handleSendTest"
                                                >
                                                    <UiSpinner v-if="isSendingTest" />
                                                    <ISend v-else />
                                                    {{ $t('instance.email.sendTest') }}
                                                </UiButton>
                                            </div>
                                            <UiFieldDescription>
                                                {{ $t('instance.email.testDescription') }}
                                            </UiFieldDescription>
                                            <UiFieldError
                                                v-if="formErrors?.to?._errors?.length"
                                                :errors="formErrors.to._errors"
                                            />
                                        </UiField>
                                    </template>

                                    <template v-if="section.id === 'ai' && aiStatus">
                                        <UiFieldSeparator class="sm:col-span-2" />
                                        <UiField
                                            orientation="horizontal"
                                            class="has-[>[data-slot=field-content]]:items-center sm:col-span-2"
                                        >
                                            <UiFieldContent>
                                                <UiFieldLabel>
                                                    {{ $t('instance.ai.queue') }}
                                                </UiFieldLabel>
                                                <UiFieldDescription>
                                                    {{
                                                        $t('instance.ai.queueStatus', {
                                                            pending: aiStatus.pending,
                                                            processing: aiStatus.processing,
                                                            failed: aiStatus.failed,
                                                            size: filesize(
                                                                aiStatus.modelCacheSize,
                                                                {
                                                                    locale,
                                                                },
                                                            ),
                                                        })
                                                    }}
                                                </UiFieldDescription>
                                            </UiFieldContent>
                                            <div class="flex flex-wrap justify-end gap-2">
                                                <UiButton
                                                    v-if="aiStatus.failed"
                                                    type="button"
                                                    variant="outline"
                                                    size="sm"
                                                    :disabled="isRetryingAi"
                                                    @click="handleRetryAi"
                                                >
                                                    <UiSpinner v-if="isRetryingAi" />
                                                    <IRotateCcw v-else />
                                                    {{ $t('instance.ai.retryFailed') }}
                                                </UiButton>
                                                <UiButton
                                                    type="button"
                                                    variant="outline"
                                                    size="sm"
                                                    @click="handleResetAi"
                                                >
                                                    <ITrash2 />
                                                    {{ $t('instance.ai.resetData') }}
                                                </UiButton>
                                            </div>
                                        </UiField>
                                    </template>
                                </UiFieldGroup>
                            </UiCardContent>
                            <UiCardFooter class="justify-end">
                                <UiButton
                                    type="submit"
                                    :disabled="!isDirty(section) || savingSection === section.id"
                                >
                                    <UiSpinner v-if="savingSection === section.id" />
                                    {{ $t('instance.save') }}
                                </UiButton>
                            </UiCardFooter>
                        </form>
                    </UiCard>
                </div>
            </template>
        </div>

        <UiCard>
            <UiCardHeader>
                <UiCardTitle>{{ $t('instance.storage.title') }}</UiCardTitle>
                <UiCardDescription>
                    {{ $t('instance.storage.description') }}
                </UiCardDescription>
                <UiCardAction>
                    <UiButton @click="handleOpenStorage()">
                        <IPlus />
                        {{ $t('instance.storage.add') }}
                    </UiButton>
                </UiCardAction>
            </UiCardHeader>
            <UiCardContent class="flex flex-col gap-2">
                <div v-if="migration" class="flex flex-col gap-2 pb-2">
                    <div class="flex items-center justify-between gap-2">
                        <TextBlock variant="small">{{ $t('instance.storage.moving') }}</TextBlock>
                        <TextBlock variant="muted" class="text-xs">
                            {{ migration.done }} / {{ migration.total }}
                        </TextBlock>
                    </div>
                    <UiProgress :model-value="(migration.done / (migration.total || 1)) * 100" />
                </div>

                <div v-if="storageImport" class="flex flex-col gap-2 pb-2">
                    <div class="flex items-center justify-between gap-2">
                        <TextBlock variant="small">
                            {{
                                storageImport.app
                                    ? $t('instance.storage.importingApp', {
                                          app: IMPORT_APPS[storageImport.app],
                                      })
                                    : $t('instance.storage.importing', {
                                          name: storageName(storageImport.storageId),
                                      })
                            }}
                        </TextBlock>
                        <TextBlock variant="muted" class="text-xs">
                            {{ storageImport.done }} / {{ storageImport.total }}
                        </TextBlock>
                    </div>
                    <UiProgress
                        :model-value="(storageImport.done / (storageImport.total || 1)) * 100"
                    />
                </div>

                <UiAlert v-else-if="importResult?.app" class="mb-2">
                    <IImport />
                    <UiAlertTitle>
                        {{
                            $t(
                                'instance.storage.importedAppTitle',
                                { n: importResult.imported, app: IMPORT_APPS[importResult.app] },
                                importResult.imported,
                            )
                        }}
                    </UiAlertTitle>
                    <UiAlertDescription>
                        {{
                            $t('instance.storage.importedAppDescription', {
                                skipped: importResult.skipped,
                                missing: importResult.missing,
                                failed: importResult.failed,
                            })
                        }}
                    </UiAlertDescription>
                </UiAlert>

                <UiAlert v-else-if="importResult" class="mb-2">
                    <IFolderInput />
                    <UiAlertTitle>
                        {{
                            $t(
                                'instance.storage.importedTitle',
                                {
                                    n: importResult.imported,
                                    name: storageName(importResult.storageId),
                                },
                                importResult.imported,
                            )
                        }}
                    </UiAlertTitle>
                    <UiAlertDescription>
                        {{
                            $t('instance.storage.importedDescription', {
                                skipped: importResult.skipped,
                                failed: importResult.failed,
                            })
                        }}
                    </UiAlertDescription>
                </UiAlert>

                <UiCard v-for="storage in storageRows" :key="storage.id ?? 'local'" size="sm">
                    <UiCardContent class="flex min-h-8 items-center gap-3">
                        <component
                            :is="storageIcons[storage.type]"
                            class="size-4 shrink-0 text-muted-foreground"
                        />
                        <div class="flex min-w-0 flex-1 flex-col">
                            <div class="flex min-w-0 items-center gap-2">
                                <TextBlock variant="small" class="truncate">
                                    {{ storage.name }}
                                </TextBlock>
                                <UiBadge variant="secondary">
                                    {{ storageTypeLabels[storage.type] }}
                                </UiBadge>
                                <UiBadge v-if="storage.id === storages!.activeStorageId">
                                    {{ $t('instance.storage.active') }}
                                </UiBadge>
                            </div>
                            <TextBlock variant="muted" class="text-xs">
                                {{
                                    $t('instance.storage.counts', {
                                        files: $t('instance.storage.fileCount', storage.files),
                                        backups: $t(
                                            'instance.storage.backupCount',
                                            storage.backups,
                                        ),
                                    })
                                }}
                            </TextBlock>
                        </div>
                        <UiButton
                            v-if="storage.id !== storages!.activeStorageId"
                            variant="outline"
                            size="sm"
                            :disabled="activatingStorage === (storage.id ?? 'local')"
                            @click="handleActivateStorage(storage)"
                        >
                            <UiSpinner v-if="activatingStorage === (storage.id ?? 'local')" />
                            {{ $t('instance.storage.setActive') }}
                        </UiButton>
                        <UiButton
                            v-else-if="hasDataElsewhere && !migration"
                            variant="outline"
                            size="sm"
                            @click="handleMigrateStorage(storage)"
                        >
                            {{ $t('instance.storage.moveHere') }}
                        </UiButton>
                        <UiDropdownMenu>
                            <UiDropdownMenuTrigger as-child>
                                <UiButton
                                    variant="ghost"
                                    size="icon-sm"
                                    :aria-label="$t('instance.storage.actions')"
                                >
                                    <IEllipsis />
                                </UiButton>
                            </UiDropdownMenuTrigger>
                            <UiDropdownMenuContent align="end" class="w-56">
                                <UiDropdownMenuItem
                                    :disabled="!!migration || !!storageImport"
                                    @select="handleOpenImport(storage)"
                                >
                                    <IFolderInput />
                                    {{ $t('instance.storage.importFiles') }}
                                </UiDropdownMenuItem>
                                <UiDropdownMenuItem
                                    :disabled="!!migration || !!storageImport"
                                    @select="handleOpenAppImport(storage)"
                                >
                                    <IImport />
                                    {{ $t('instance.storage.importFromApp') }}
                                </UiDropdownMenuItem>
                                <template v-if="storage.id">
                                    <UiDropdownMenuItem @select="handleOpenStorage(storage)">
                                        <IPencil />
                                        {{ $t('instance.storage.edit') }}
                                    </UiDropdownMenuItem>
                                    <UiDropdownMenuSeparator />
                                    <UiDropdownMenuItem
                                        variant="destructive"
                                        :disabled="
                                            storage.id === storages!.activeStorageId ||
                                            !!storage.files ||
                                            !!storage.backups
                                        "
                                        @select="handleDeleteStorage(storage)"
                                    >
                                        <ITrash2 />
                                        {{ $t('instance.storage.delete') }}
                                    </UiDropdownMenuItem>
                                </template>
                            </UiDropdownMenuContent>
                        </UiDropdownMenu>
                    </UiCardContent>
                </UiCard>
            </UiCardContent>
        </UiCard>
    </div>

    <AppImportDialog
        v-model:open="appImportDialog.open"
        :storage="appImportDialog.storage"
        @started="importResult = null"
    />

    <UiDialog v-model:open="importDialog.open">
        <UiDialogContent class="sm:max-w-lg">
            <UiDialogHeader>
                <UiDialogTitle>
                    {{ $t('instance.import.title', { name: importDialog.storage?.name }) }}
                </UiDialogTitle>
                <UiDialogDescription>
                    {{ $t('instance.import.description') }}
                </UiDialogDescription>
            </UiDialogHeader>

            <div class="flex flex-col gap-3">
                <TextBlock variant="small">{{ $t('instance.import.howItWorks') }}</TextBlock>
                <template v-if="!importDialog.storage?.id">
                    <TextBlock variant="muted" class="text-sm">
                        <i18n-t keypath="instance.import.localHint" scope="global">
                            <template #folder>
                                <code>{{ UPLOADS_FOLDER }}</code>
                            </template>
                        </i18n-t>
                    </TextBlock>
                    <UiInputGroup v-if="importPreview?.importPath">
                        <UiInputGroupInput
                            :model-value="importPreview.importPath"
                            :aria-label="$t('instance.import.folderLabel')"
                            readonly
                        />
                        <UiInputGroupAddon align="inline-end">
                            <UiInputGroupButton
                                size="icon-xs"
                                type="button"
                                :aria-label="$t('instance.import.copyPath')"
                                @click="handleCopyImportPath"
                            >
                                <ICopy />
                            </UiInputGroupButton>
                        </UiInputGroupAddon>
                    </UiInputGroup>
                </template>
                <TextBlock v-else variant="muted" class="text-sm">
                    <i18n-t keypath="instance.import.storageHint" scope="global">
                        <template #folder>
                            <code>{{ BACKUPS_FOLDER }}</code>
                        </template>
                    </i18n-t>
                </TextBlock>
                <ul class="flex list-disc flex-col gap-1 pl-5 text-sm text-muted-foreground">
                    <i18n-t keypath="instance.import.ruleMoved" tag="li" scope="global">
                        <template #folder>
                            <code>{{ UPLOADS_FOLDER }}</code>
                        </template>
                    </i18n-t>
                    <li>{{ $t('instance.import.ruleFolders', { max: MAX_FOLDER_DEPTH }) }}</li>
                    <li>{{ $t('instance.import.ruleDuplicates') }}</li>
                    <li>{{ $t('instance.import.ruleRecent') }}</li>
                    <li>{{ $t('instance.import.ruleQuota') }}</li>
                </ul>
            </div>

            <div
                v-if="importDialog.isLoading"
                class="flex items-center gap-2 text-sm text-muted-foreground"
            >
                <UiSpinner />
                {{ $t('instance.import.searching') }}
            </div>

            <EmptyState
                v-else-if="importDialog.error"
                :icon="ICircleAlert"
                :title="$t('instance.import.readFailed')"
                :description="importDialog.error"
            >
                <UiButton variant="outline" @click="loadImportPreview">
                    {{ $t('instance.import.tryAgain') }}
                </UiButton>
            </EmptyState>

            <EmptyState
                v-else-if="importPreview && !importPreview.count"
                :icon="IFolderSearch"
                :title="$t('instance.import.nothingTitle')"
                :description="
                    importDialog.storage?.id
                        ? $t('instance.import.nothingStorage')
                        : $t('instance.import.nothingLocal')
                "
            >
                <UiButton variant="outline" @click="loadImportPreview">
                    {{ $t('instance.import.checkAgain') }}
                </UiButton>
            </EmptyState>

            <UiField v-else-if="importPreview">
                <UiFieldLabel for="import-user">{{ $t('instance.import.into') }}</UiFieldLabel>
                <UserPicker
                    id="import-user"
                    :label="importDialog.user?.username ?? $t('instance.import.chooseUser')"
                    :selected="importDialog.user?.id"
                    @select="importDialog.user = $event"
                />
                <UiFieldDescription>
                    {{
                        $t(
                            'instance.import.found',
                            {
                                n: importPreview.count,
                                size: filesize(importPreview.size, { locale }),
                            },
                            importPreview.count,
                        )
                    }}
                </UiFieldDescription>
            </UiField>

            <UiDialogFooter>
                <UiButton variant="outline" @click="importDialog.open = false">
                    {{ $t('common.cancel') }}
                </UiButton>
                <UiButton
                    :disabled="!importPreview?.count || !importDialog.user || isStartingImport"
                    @click="handleStartImport"
                >
                    <UiSpinner v-if="isStartingImport" />
                    {{ $t('instance.import.start') }}
                </UiButton>
            </UiDialogFooter>
        </UiDialogContent>
    </UiDialog>

    <UiDialog v-model:open="storageDialog.open">
        <UiDialogContent class="sm:max-w-lg">
            <form class="flex flex-col gap-6" @submit.prevent="handleSaveStorage">
                <UiDialogHeader>
                    <UiDialogTitle>
                        {{
                            storageDialog.id
                                ? $t('instance.storageDialog.editTitle')
                                : $t('instance.storageDialog.addTitle')
                        }}
                    </UiDialogTitle>
                    <UiDialogDescription>
                        {{ $t('instance.storageDialog.description') }}
                    </UiDialogDescription>
                </UiDialogHeader>

                <UiFieldGroup class="grid gap-x-4 sm:grid-cols-2">
                    <UiField :data-invalid="!!storageErrors?.name">
                        <UiFieldLabel for="storage-name">
                            {{ $t('instance.storageDialog.name') }}
                        </UiFieldLabel>
                        <UiInput
                            id="storage-name"
                            v-model="storageForm.name"
                            :placeholder="$t('instance.storageDialog.namePlaceholder')"
                        />
                        <UiFieldError
                            v-if="storageErrors?.name"
                            :errors="storageErrors.name._errors"
                        />
                    </UiField>

                    <UiField>
                        <UiFieldLabel for="storage-type">
                            {{ $t('instance.storageDialog.type') }}
                        </UiFieldLabel>
                        <UiSelect
                            :model-value="storageForm.type"
                            :disabled="!!storageDialog.id"
                            @update:model-value="handleStorageType($event as StorageType)"
                        >
                            <UiSelectTrigger id="storage-type" class="w-full">
                                <UiSelectValue />
                            </UiSelectTrigger>
                            <UiSelectContent>
                                <UiSelectItem
                                    v-for="type in Object.values(StorageType)"
                                    :key="type"
                                    :value="type"
                                >
                                    {{ storageTypeLabels[type] }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                    </UiField>

                    <template v-for="field in storageFields[storageForm.type]" :key="field.key">
                        <UiField
                            v-if="field.type === 'switch'"
                            orientation="horizontal"
                            class="has-[>[data-slot=field-content]]:items-center sm:col-span-2"
                        >
                            <UiFieldContent>
                                <UiFieldLabel :for="`storage-${field.key}`">
                                    {{ $t(field.label) }}
                                </UiFieldLabel>
                                <UiFieldDescription>
                                    {{ $t(field.description!) }}
                                </UiFieldDescription>
                            </UiFieldContent>
                            <UiSwitch
                                :id="`storage-${field.key}`"
                                v-model="storageForm.config[field.key]"
                            />
                        </UiField>

                        <UiField
                            v-else
                            :class="!field.half && 'sm:col-span-2'"
                            :data-invalid="!!storageErrors?.config?.[field.key]"
                        >
                            <UiFieldLabel :for="`storage-${field.key}`">
                                {{ $t(field.label) }}
                            </UiFieldLabel>

                            <PasswordInput
                                v-if="field.type === 'secret'"
                                :id="`storage-${field.key}`"
                                v-model="storageForm.config[field.key]"
                                autocomplete="off"
                                :placeholder="
                                    storageDialog.secrets[field.key]
                                        ? $t('instance.leaveEmptyToKeep')
                                        : ''
                                "
                            />

                            <UiTextarea
                                v-else-if="field.type === 'key'"
                                :id="`storage-${field.key}`"
                                v-model="storageForm.config[field.key]"
                                class="max-h-32 font-mono text-xs"
                                :placeholder="
                                    storageDialog.secrets[field.key]
                                        ? $t('instance.leaveEmptyToKeep')
                                        : '-----BEGIN OPENSSH PRIVATE KEY-----'
                                "
                            />

                            <UiNumberField
                                v-else-if="field.type === 'number'"
                                :id="`storage-${field.key}`"
                                v-model="storageForm.config[field.key]"
                                :min="1"
                                :max="65535"
                                :format-options="{ useGrouping: false }"
                            >
                                <UiNumberFieldContent>
                                    <UiNumberFieldDecrement />
                                    <UiNumberFieldInput />
                                    <UiNumberFieldIncrement />
                                </UiNumberFieldContent>
                            </UiNumberField>

                            <UiInput
                                v-else
                                :id="`storage-${field.key}`"
                                v-model="storageForm.config[field.key]"
                                :placeholder="field.placeholder"
                            />

                            <UiFieldDescription v-if="field.description">
                                {{ $t(field.description!) }}
                            </UiFieldDescription>
                            <UiFieldError
                                v-if="storageErrors?.config?.[field.key]?._errors?.length"
                                :errors="storageErrors.config[field.key]._errors"
                            />
                        </UiField>
                    </template>
                </UiFieldGroup>

                <UiDialogFooter>
                    <UiButton
                        type="button"
                        variant="outline"
                        :disabled="isTestingStorage"
                        @click="handleTestStorage"
                    >
                        <UiSpinner v-if="isTestingStorage" />
                        {{ $t('instance.storageDialog.test') }}
                    </UiButton>
                    <UiButton type="submit" :disabled="isSavingStorage">
                        <UiSpinner v-if="isSavingStorage" />
                        {{ $t('instance.save') }}
                    </UiButton>
                </UiDialogFooter>
            </form>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';

import { ICircleAlert, ICloud, IFolderSearch, IHardDrive, IServer } from '#components';
import { StorageType } from '#shared/prisma/enums';

type FieldType = 'text' | 'secret' | 'number' | 'switch' | 'select' | 'permissions';

type SettingField = {
    key: InstanceSettingKey;
    label: string;
    type: FieldType;
    description?: string;
    placeholder?: string;
    min?: number;
    max?: number;
    formatOptions?: Intl.NumberFormatOptions;
    options?: { value: string; label: string }[];
    half?: boolean;
};

type SettingSection = {
    id: string;
    fields: SettingField[];
};

type StorageField = {
    key: string;
    label: string;
    type: 'text' | 'secret' | 'key' | 'number' | 'switch';
    placeholder?: string;
    description?: string;
    half?: boolean;
};

const UPLOADS_FOLDER = 'uploads';
const BACKUPS_FOLDER = 'backups';

const { $toast } = useNuxtApp();
const { t, locale } = useI18n();

const formErrors = ref();
const savingSection = ref<string>();
const resettingKey = ref<InstanceSettingKey>();
const secrets = reactive({} as Record<SecretSettingKey, string>);
const testEmail = ref('');
const isSendingTest = ref(false);

const { data, refresh } = await useFetch('/api/settings');
const { data: aiStatus, refresh: refreshAiStatus } = await useFetch('/api/settings/ai-status');
const isRetryingAi = ref(false);

useIntervalFn(refreshAiStatus, 10_000);

const form = reactive({} as InstanceSettings);

watch(
    data,
    (value) => {
        if (value) Object.assign(form, structuredClone(toRaw(value.values)));
        for (const key of secretSettingKeys) secrets[key] = '';
    },
    { immediate: true },
);

const registrablePermissions = [
    { value: 'UploadFiles', label: 'instance.permissions.UploadFiles' },
    { value: 'TakeNotes', label: 'instance.permissions.TakeNotes' },
    { value: 'ShortenLinks', label: 'instance.permissions.ShortenLinks' },
] as const;

const sections: SettingSection[] = [
    {
        id: 'general',
        fields: [
            { key: 'siteName', label: 'instance.fields.siteName', type: 'text' },
            {
                key: 'returnHttps',
                label: 'instance.fields.returnHttps',
                type: 'select',
                description: 'instance.fields.returnHttpsDescription',
                options: [
                    { value: 'auto', label: 'instance.fields.returnHttpsAuto' },
                    { value: 'true', label: 'instance.fields.returnHttpsTrue' },
                    { value: 'false', label: 'instance.fields.returnHttpsFalse' },
                ],
            },
        ],
    },
    {
        id: 'uploads',
        fields: [
            {
                key: 'fileChunkSize',
                label: 'instance.fields.fileChunkSize',
                type: 'number',
                min: 5,
                max: 100,
                description: 'instance.fields.fileChunkSizeDescription',
                half: true,
            },
            {
                key: 'avatarMaxSize',
                label: 'instance.fields.avatarMaxSize',
                type: 'number',
                min: 1,
                max: 100,
                half: true,
            },
            {
                key: 'trashRetentionDays',
                label: 'instance.fields.trashRetentionDays',
                type: 'number',
                min: 0,
                max: 365,
                description: 'instance.fields.trashRetentionDaysDescription',
            },
            {
                key: 'removeExifData',
                label: 'instance.fields.removeExifData',
                type: 'switch',
                description: 'instance.fields.removeExifDataDescription',
            },
            {
                key: 'scheduledBackupsEnabled',
                label: 'instance.fields.scheduledBackupsEnabled',
                type: 'switch',
                description: 'instance.fields.scheduledBackupsEnabledDescription',
            },
        ],
    },
    {
        id: 'registration',
        fields: [
            {
                key: 'registrationEnabled',
                label: 'instance.fields.registrationEnabled',
                type: 'switch',
                description: 'instance.fields.registrationEnabledDescription',
            },
            {
                key: 'registrationPermissions',
                label: 'instance.fields.registrationPermissions',
                type: 'permissions',
            },
            {
                key: 'registrationUsableSpace',
                label: 'instance.fields.registrationUsableSpace',
                type: 'number',
                min: -1,
                description: 'instance.fields.registrationUsableSpaceDescription',
            },
        ],
    },
    {
        id: 'turnstile',
        fields: [
            { key: 'turnstileSiteKey', label: 'instance.fields.turnstileSiteKey', type: 'text' },
            {
                key: 'turnstileSecretKey',
                label: 'instance.fields.turnstileSecretKey',
                type: 'secret',
                description: 'instance.fields.turnstileSecretKeyDescription',
            },
        ],
    },
    {
        id: 'ai',
        fields: [
            {
                key: 'aiEnabled',
                label: 'instance.fields.aiEnabled',
                type: 'switch',
                description: 'instance.fields.aiEnabledDescription',
            },
            {
                key: 'aiPreset',
                label: 'instance.fields.aiPreset',
                type: 'select',
                options: [
                    { value: 'low', label: 'instance.fields.aiPresetLow' },
                    { value: 'medium', label: 'instance.fields.aiPresetMedium' },
                    { value: 'high', label: 'instance.fields.aiPresetHigh' },
                ],
                description: 'instance.fields.aiPresetDescription',
            },
            {
                key: 'aiQueueConcurrency',
                label: 'instance.fields.aiQueueConcurrency',
                type: 'number',
                min: 1,
                max: 8,
                half: true,
            },
            {
                key: 'aiInferenceThreads',
                label: 'instance.fields.aiInferenceThreads',
                type: 'number',
                min: 1,
                max: 64,
                half: true,
            },
            {
                key: 'aiJobLeaseMs',
                label: 'instance.fields.aiJobLeaseMs',
                type: 'number',
                min: 60_000,
                description: 'instance.fields.aiJobLeaseMsDescription',
            },
        ],
    },
    {
        id: 'email',
        fields: [
            {
                key: 'siteUrl',
                label: 'instance.fields.siteUrl',
                type: 'text',
                placeholder: 'https://files.example.com',
                description: 'instance.fields.siteUrlDescription',
            },
            {
                key: 'smtpHost',
                label: 'instance.fields.smtpHost',
                type: 'text',
                placeholder: 'smtp.example.com',
                half: true,
            },
            {
                key: 'smtpPort',
                label: 'instance.fields.smtpPort',
                type: 'number',
                min: 1,
                max: 65535,
                formatOptions: { useGrouping: false },
                half: true,
            },
            {
                key: 'smtpSecurity',
                label: 'instance.fields.smtpSecurity',
                type: 'select',
                options: [
                    { value: 'starttls', label: 'instance.fields.smtpSecurityStarttls' },
                    { value: 'tls', label: 'instance.fields.smtpSecurityTls' },
                    { value: 'none', label: 'instance.fields.smtpSecurityNone' },
                ],
                description: 'instance.fields.smtpSecurityDescription',
            },
            { key: 'smtpUser', label: 'instance.fields.smtpUser', type: 'text', half: true },
            {
                key: 'smtpPassword',
                label: 'instance.fields.smtpPassword',
                type: 'secret',
                half: true,
            },
            {
                key: 'smtpFrom',
                label: 'instance.fields.smtpFrom',
                type: 'text',
                placeholder: 'fileship@example.com',
                description: 'instance.fields.smtpFromDescription',
            },
        ],
    },
    {
        id: 'logs',
        fields: [
            {
                key: 'logLevel',
                label: 'instance.fields.logLevel',
                type: 'select',
                options: [
                    { value: '0', label: 'instance.fields.logLevelNothing' },
                    { value: '1', label: 'instance.fields.logLevelImportant' },
                    { value: '2', label: 'instance.fields.logLevelEverything' },
                ],
            },
            {
                key: 'telemetryEnabled',
                label: 'instance.fields.telemetryEnabled',
                type: 'switch',
                description: 'instance.fields.telemetryEnabledDescription',
            },
        ],
    },
    {
        id: 'webhooks',
        fields: [
            {
                key: 'webhookPrivateTargets',
                label: 'instance.fields.webhookPrivateTargets',
                type: 'switch',
                description: 'instance.fields.webhookPrivateTargetsDescription',
            },
        ],
    },
];

const columns = [
    [sections[0]!, sections[1]!, sections[2]!, sections[5]!],
    [sections[4]!, sections[3]!, sections[6]!, sections[7]!],
];

const storageTypeLabels = computed(() => ({
    Local: t('instance.storage.local'),
    [StorageType.S3]: 'S3',
    [StorageType.SFTP]: 'SFTP',
    [StorageType.FTP]: 'FTP',
}));

const storageIcons = {
    Local: IHardDrive,
    [StorageType.S3]: ICloud,
    [StorageType.SFTP]: IServer,
    [StorageType.FTP]: IServer,
};

const storageFields: Record<StorageType, StorageField[]> = {
    [StorageType.S3]: [
        {
            key: 'endpoint',
            label: 'instance.storageFields.endpoint',
            type: 'text',
            placeholder: 'https://<account>.r2.cloudflarestorage.com',
            description: 'instance.storageFields.endpointDescription',
        },
        {
            key: 'region',
            label: 'instance.storageFields.region',
            type: 'text',
            placeholder: 'us-east-1',
            half: true,
        },
        { key: 'bucket', label: 'instance.storageFields.bucket', type: 'text', half: true },
        {
            key: 'accessKeyId',
            label: 'instance.storageFields.accessKeyId',
            type: 'text',
            half: true,
        },
        {
            key: 'secretAccessKey',
            label: 'instance.storageFields.secretAccessKey',
            type: 'secret',
            half: true,
        },
        {
            key: 'prefix',
            label: 'instance.storageFields.prefix',
            type: 'text',
            placeholder: 'fileship',
            description: 'instance.storageFields.prefixDescription',
        },
        {
            key: 'forcePathStyle',
            label: 'instance.storageFields.forcePathStyle',
            type: 'switch',
            description: 'instance.storageFields.forcePathStyleDescription',
        },
    ],
    [StorageType.SFTP]: [
        { key: 'host', label: 'instance.storageFields.host', type: 'text', half: true },
        { key: 'port', label: 'instance.storageFields.port', type: 'number', half: true },
        { key: 'username', label: 'instance.storageFields.username', type: 'text', half: true },
        { key: 'password', label: 'instance.storageFields.password', type: 'secret', half: true },
        {
            key: 'privateKey',
            label: 'instance.storageFields.privateKey',
            type: 'key',
            description: 'instance.storageFields.privateKeyDescription',
        },
        {
            key: 'root',
            label: 'instance.storageFields.root',
            type: 'text',
            placeholder: '/srv/fileship',
        },
    ],
    [StorageType.FTP]: [
        { key: 'host', label: 'instance.storageFields.host', type: 'text', half: true },
        { key: 'port', label: 'instance.storageFields.port', type: 'number', half: true },
        { key: 'user', label: 'instance.storageFields.username', type: 'text', half: true },
        { key: 'password', label: 'instance.storageFields.password', type: 'secret', half: true },
        {
            key: 'root',
            label: 'instance.storageFields.root',
            type: 'text',
            placeholder: '/fileship',
        },
        {
            key: 'secure',
            label: 'instance.storageFields.secure',
            type: 'switch',
            description: 'instance.storageFields.secureDescription',
        },
    ],
};

const storageDefaults: Record<StorageType, Record<string, any>> = {
    [StorageType.S3]: {
        endpoint: '',
        region: 'us-east-1',
        bucket: '',
        prefix: '',
        accessKeyId: '',
        secretAccessKey: '',
        forcePathStyle: false,
    },
    [StorageType.SFTP]: {
        host: '',
        port: 22,
        username: '',
        password: '',
        privateKey: '',
        root: '/',
    },
    [StorageType.FTP]: {
        host: '',
        port: 21,
        user: '',
        password: '',
        secure: false,
        root: '/',
    },
};

const migration = useStorageMigration();
const storageErrors = ref();
const isTestingStorage = ref(false);
const isSavingStorage = ref(false);
const activatingStorage = ref<string>();

const storageDialog = reactive({
    open: false,
    id: null as string | null,
    secrets: {} as Record<string, boolean>,
});

const storageForm = reactive({
    name: '',
    type: StorageType.S3 as StorageType,
    config: {} as Record<string, any>,
});

const { data: storages, refresh: refreshStorages } = await useFetch('/api/storages');

migration.value = storages.value?.migration ?? null;

watch(migration, (value, oldValue) => {
    if (!value && oldValue) refreshStorages();
});

const storageImport = useStorageImport();
const importResult = ref<StorageImportProgress | null>(null);

storageImport.value = storages.value?.import ?? null;

watch(storageImport, (value, oldValue) => {
    if (value || !oldValue) return;

    importResult.value = oldValue;
    refreshStorages();
});

type StorageRow = NonNullable<typeof storages.value>['items'][number];

const storageRows = computed(() => [
    {
        id: null,
        name: t('instance.storage.dataDirectory'),
        type: 'Local' as const,
        ...storages.value!.local,
    },
    ...storages.value!.items,
]);

const hasDataElsewhere = computed(() =>
    storageRows.value.some(
        (storage) =>
            storage.id !== storages.value!.activeStorageId && (storage.files || storage.backups),
    ),
);

const isOverridden = (key: InstanceSettingKey) => !!data.value?.overridden.includes(key);

const formatDefault = (field: SettingField) => {
    const value = data.value!.defaults[field.key];

    if (field.type === 'secret') return t('instance.values.none');
    if (typeof value === 'boolean')
        return value ? t('instance.values.on') : t('instance.values.off');
    if (field.options) return t(field.options.find((o) => o.value === String(value))!.label);
    if (Array.isArray(value)) {
        return value.length
            ? value.map((permission) => t(`instance.permissions.${permission}`)).join(', ')
            : t('instance.values.none');
    }
    if (typeof value === 'number') return new Intl.NumberFormat(locale.value).format(value);

    return String(value || t('instance.values.empty'));
};

const getChanges = (section: SettingSection) => {
    const changes: Record<string, unknown> = {};

    for (const field of section.fields) {
        if (field.type === 'secret') {
            const value = secrets[field.key as SecretSettingKey].trim();
            if (value) changes[field.key] = value;
        } else if (
            JSON.stringify(form[field.key]) !== JSON.stringify(data.value!.values[field.key])
        ) {
            changes[field.key] = form[field.key];
        }
    }

    return changes;
};

const isDirty = (section: SettingSection) => !!Object.keys(getChanges(section)).length;

const handleSelect = (key: InstanceSettingKey, value: string) => {
    (form as any)[key] = key === 'logLevel' ? Number(value) : value;
};

const handleTogglePermission = (permission: string) => {
    form.registrationPermissions = form.registrationPermissions.includes(permission as never)
        ? form.registrationPermissions.filter((p) => p !== permission)
        : [...form.registrationPermissions, permission as never];
};

const handleSave = async (section: SettingSection) => {
    savingSection.value = section.id;
    formErrors.value = null;

    try {
        const result = await withVerification((verificationData) =>
            $fetch('/api/settings', {
                method: 'PATCH',
                body: { ...getChanges(section), verificationData },
            }),
        );

        if (result) {
            await refresh();
            $toast.success(t(`instance.sections.${section.id}.saved`));
        }
    } catch (error: any) {
        formErrors.value = error.data?.data?.formErrors;
        if (!formErrors.value) $toast.error(getErrorMessage(error));
    }

    savingSection.value = undefined;
};

const handleSendTest = async () => {
    isSendingTest.value = true;
    formErrors.value = null;

    try {
        const result = await withVerification((verificationData) =>
            $fetch('/api/settings/email-test', {
                method: 'POST',
                body: {
                    to: testEmail.value,
                    smtpHost: form.smtpHost,
                    smtpPort: form.smtpPort,
                    smtpSecurity: form.smtpSecurity,
                    smtpUser: form.smtpUser,
                    smtpPassword: secrets.smtpPassword.trim(),
                    smtpFrom: form.smtpFrom,
                    verificationData,
                },
            }),
        );

        if (result) $toast.success(t('instance.email.testSent', { email: testEmail.value }));
    } catch (error: any) {
        formErrors.value = error.data?.data?.formErrors;
        if (!formErrors.value) $toast.error(getErrorMessage(error));
    }

    isSendingTest.value = false;
};

const handleRetryAi = async () => {
    isRetryingAi.value = true;

    try {
        const { retried } = await $fetch('/api/settings/ai-retry', { method: 'POST' });
        await refreshAiStatus();
        $toast.success(t('instance.ai.retrying', retried));
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    isRetryingAi.value = false;
};

const handleResetAi = () =>
    askConfirmation({
        title: t('instance.ai.resetTitle'),
        description: t('instance.ai.resetDescription'),
        confirmText: t('instance.ai.resetConfirm'),
        destructive: true,
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch('/api/settings/ai-reset', {
                        method: 'POST',
                        body: { verificationData },
                    }),
                );

                if (result) {
                    await refreshAiStatus();
                    $toast.success(t('instance.ai.analyzingAgain', result.files));
                }
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
                return false;
            }
        },
    });

const handleReset = async (key: InstanceSettingKey) => {
    resettingKey.value = key;

    try {
        const result = await withVerification((verificationData) =>
            $fetch('/api/settings', {
                method: 'PATCH',
                body: { [key]: null, verificationData },
            }),
        );

        if (result) {
            await refresh();
            $toast.success(t('instance.resetDone'));
        }
    } catch (error) {
        $toast.error(getErrorMessage(error));
    }

    resettingKey.value = undefined;
};

const handleOpenStorage = (storage?: StorageRow) => {
    storageErrors.value = null;
    storageDialog.id = storage?.id ?? null;
    storageDialog.secrets = storage?.secrets ?? {};
    storageForm.name = storage?.name ?? '';
    storageForm.type = storage?.type ?? StorageType.S3;
    storageForm.config = { ...storageDefaults[storageForm.type], ...storage?.config };
    storageDialog.open = true;
};

const handleStorageType = (type: StorageType) => {
    storageForm.type = type;
    storageForm.config = { ...storageDefaults[type] };
};

const handleTestStorage = async () => {
    isTestingStorage.value = true;
    storageErrors.value = null;

    try {
        const result = await withVerification((verificationData) =>
            $fetch('/api/storages/test', {
                method: 'POST',
                body: { ...storageForm, id: storageDialog.id ?? undefined, verificationData },
            }),
        );

        if (result) $toast.success(t('instance.storage.connectionWorks'));
    } catch (error: any) {
        storageErrors.value = error.data?.data?.formErrors;
        if (!storageErrors.value) $toast.error(getErrorMessage(error));
    }

    isTestingStorage.value = false;
};

const handleSaveStorage = async () => {
    isSavingStorage.value = true;
    storageErrors.value = null;

    try {
        const result = await withVerification((verificationData) =>
            storageDialog.id
                ? $fetch(`/api/storages/${storageDialog.id}`, {
                      method: 'PATCH',
                      body: {
                          name: storageForm.name,
                          config: storageForm.config,
                          verificationData,
                      },
                  })
                : $fetch('/api/storages', {
                      method: 'POST',
                      body: { ...storageForm, verificationData },
                  }),
        );

        if (result) {
            storageDialog.open = false;
            await refreshStorages();
            $toast.success(t('instance.storage.saved'));
        }
    } catch (error: any) {
        storageErrors.value = error.data?.data?.formErrors;
        if (!storageErrors.value) $toast.error(getErrorMessage(error));
    }

    isSavingStorage.value = false;
};

const handleActivateStorage = async (storage: { id: string | null; name: string }) => {
    activatingStorage.value = storage.id ?? 'local';

    try {
        const result = await withVerification((verificationData) =>
            $fetch('/api/storages/active', {
                method: 'PUT',
                body: { id: storage.id, verificationData },
            }),
        );

        if (result) {
            await refreshStorages();
            $toast.success(t('instance.storage.activated', { name: storage.name }));
        }
    } catch (error) {
        $toast.error(getErrorMessage(error));
    }

    activatingStorage.value = undefined;
};

const handleMigrateStorage = async (storage: { name: string }) => {
    await askConfirmation({
        title: t('instance.storage.migrateTitle', { name: storage.name }),
        description: t('instance.storage.migrateDescription'),
        confirmText: t('instance.storage.migrateConfirm'),
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch('/api/storages/migrate', {
                        method: 'POST',
                        body: { verificationData },
                    }),
                );

                if (result) $toast.success(t('instance.storage.migrating'));
            } catch (error) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};

const storageName = (storageId: string | null) =>
    storageRows.value.find((storage) => storage.id === storageId)?.name ??
    t('instance.storage.unknown');

const currentUser = useAuthUser();
const { copy } = useClipboard({ legacy: true });

const importDialog = reactive({
    open: false,
    storage: null as { id: string | null; name: string } | null,
    user: null as { id: string; username: string } | null,
    isLoading: false,
    error: '',
});

const importPreview = ref<{ count: number; size: number; importPath: string | null } | null>(null);
const isStartingImport = ref(false);

const appImportDialog = reactive({
    open: false,
    storage: null as (typeof storageRows.value)[number] | null,
});

const loadImportPreview = async () => {
    importDialog.isLoading = true;
    importDialog.error = '';

    try {
        importPreview.value = await $fetch('/api/storages/import/preview', {
            method: 'POST',
            body: { storageId: importDialog.storage!.id },
        });
    } catch (error) {
        importDialog.error = getErrorMessage(error);
    }

    importDialog.isLoading = false;
};

const handleOpenImport = (storage: { id: string | null; name: string }) => {
    importDialog.storage = storage;
    importDialog.user = currentUser.value
        ? { id: currentUser.value.id, username: currentUser.value.username }
        : null;
    importDialog.open = true;
    importPreview.value = null;

    loadImportPreview();
};

const handleOpenAppImport = (storage: (typeof storageRows.value)[number]) => {
    appImportDialog.storage = storage;
    appImportDialog.open = true;
};

const handleCopyImportPath = async () => {
    await copy(importPreview.value!.importPath!);
    $toast.success(t('instance.import.pathCopied'));
};

const handleStartImport = async () => {
    isStartingImport.value = true;

    try {
        const result = await withVerification((verificationData) =>
            $fetch('/api/storages/import', {
                method: 'POST',
                body: {
                    storageId: importDialog.storage!.id,
                    userId: importDialog.user!.id,
                    verificationData,
                },
            }),
        );

        if (result) {
            importResult.value = null;
            importDialog.open = false;
            $toast.success(t('instance.import.started'));
        }
    } catch (error) {
        $toast.error(getErrorMessage(error));
    }

    isStartingImport.value = false;
};

const handleDeleteStorage = async (storage: StorageRow) => {
    await askConfirmation({
        title: t('instance.storage.deleteTitle', { name: storage.name }),
        description: t('instance.storage.deleteDescription'),
        confirmText: t('instance.storage.delete'),
        destructive: true,
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch(`/api/storages/${storage.id}`, {
                        method: 'DELETE',
                        body: { verificationData },
                    }),
                );

                if (result) {
                    await refreshStorages();
                    $toast.success(t('instance.storage.deleted'));
                }
            } catch (error) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};

definePageMeta({
    layout: 'dashboard',
    middleware: 'super-admin-only',
});
</script>
