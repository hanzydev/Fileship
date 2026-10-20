import { app, Menu, nativeImage, Tray } from 'electron';

import type { RecorderPhase } from '../shared/ipc';
import { isConnected, runAction } from './actions';
import { openDashboard } from './dashboard';
import { openHarbor } from './harbor';
import { t } from './i18n';
import { native } from './native';
import {
    abortRecording,
    getRecorderState,
    pauseRecording,
    resumeRecording,
    stopRecording,
} from './recording/recorder';
import { getSettings, getStore } from './store';
import { checkForUpdates, getUpdateStatus, installUpdate } from './updater';
import { getUploadSummary } from './upload';
import { iconPath, markQuitting, showMainWindow } from './windows';

let tray: Tray | null = null;
let clickTimer: NodeJS.Timeout | undefined;

const RECORDING_TOOLTIPS: Record<Exclude<RecorderPhase, 'idle'>, string> = {
    preparing: 'tray.startingTooltip',
    countdown: 'tray.startingTooltip',
    recording: 'tray.recordingTooltip',
    paused: 'tray.pausedTooltip',
    processing: 'tray.processingTooltip',
};

const buildMenu = () => {
    const connected = isConnected();
    const update = getUpdateStatus();

    const recorder = getRecorderState();
    const isRecording = recorder.phase !== 'idle';

    const recordingItems: Electron.MenuItemConstructorOptions[] = isRecording
        ? [
              {
                  label:
                      recorder.kind === 'gif'
                          ? t('tray.stopGifRecording')
                          : t('tray.stopRecording'),
                  enabled: recorder.phase !== 'processing',
                  click: () => stopRecording(),
              },
              recorder.phase === 'paused'
                  ? { label: t('tray.resume'), click: () => resumeRecording() }
                  : {
                        label: t('tray.pause'),
                        enabled: recorder.phase === 'recording',
                        click: () => pauseRecording(),
                    },
              {
                  label: t('tray.discardRecording'),
                  enabled: recorder.phase !== 'processing',
                  click: () => abortRecording(),
              },
              { type: 'separator' },
          ]
        : [];

    return Menu.buildFromTemplate([
        ...recordingItems,
        {
            label: t('tray.capture'),
            submenu: [
                { label: t('tray.region'), click: () => runAction('captureRegion') },
                { label: t('tray.entireScreen'), click: () => runAction('captureFullscreen') },
                { label: t('tray.activeWindow'), click: () => runAction('captureWindow') },
                { label: t('tray.activeMonitor'), click: () => runAction('captureMonitor') },
                { label: t('tray.lastRegion'), click: () => runAction('captureLastRegion') },
                { type: 'separator' },
                {
                    label: t('tray.textFromRegion'),
                    enabled: connected,
                    click: () => runAction('captureText'),
                },
            ],
        },
        {
            label: t('tray.record'),
            enabled: !isRecording,
            submenu: [
                { label: t('tray.screenRecording'), click: () => runAction('recordVideo') },
                { label: t('tray.gif'), click: () => runAction('recordGif') },
            ],
        },
        {
            label: t('tray.upload'),
            enabled: connected,
            submenu: [
                { label: t('tray.file'), click: () => runAction('uploadFile') },
                { label: t('tray.fromClipboard'), click: () => runAction('uploadClipboard') },
                {
                    label: t('tray.shortenFromClipboard'),
                    click: () => runAction('shortenClipboard'),
                },
            ],
        },
        { type: 'separator' },
        { label: t('tray.openFileship'), click: () => showMainWindow() },
        {
            label: t('tray.openHarbor'),
            enabled: connected && getSettings().harbor.enabled,
            click: () => openHarbor(),
        },
        {
            label: t('tray.openDashboard'),
            enabled: connected,
            click: () => openDashboard(),
        },
        { type: 'separator' },
        update.state === 'ready'
            ? {
                  label: t('tray.restartToUpdate', { version: update.version! }),
                  click: installUpdate,
              }
            : { label: t('tray.checkForUpdates'), click: () => checkForUpdates() },
        { label: t('tray.settings'), click: () => showMainWindow('/settings/general') },
        { type: 'separator' },
        {
            label: t('tray.quit'),
            click: () => {
                markQuitting();
                app.quit();
            },
        },
    ]);
};

export const refreshTray = () => {
    if (!tray) return;

    tray.setContextMenu(buildMenu());

    const uploads = getUploadSummary();
    const tooltip = getStore().user ? `Fileship · ${getStore().siteName}` : 'Fileship';

    const { phase } = getRecorderState();
    if (phase !== 'idle') return tray.setToolTip(t(RECORDING_TOOLTIPS[phase]));

    tray.setToolTip(
        uploads
            ? t('tray.uploadingTooltip', { count: uploads.count, progress: uploads.progress })
            : tooltip,
    );
};

export const createTray = () => {
    tray = new Tray(nativeImage.createFromPath(iconPath()).resize({ width: 16, height: 16 }));

    tray.on('click', () => {
        clearTimeout(clickTimer);
        clickTimer = setTimeout(() => {
            if (getRecorderState().phase === 'idle') return runAction('captureRegion');
            stopRecording();
        }, native.doubleClickTime());
    });
    tray.on('middle-click', () => runAction('uploadClipboard'));
    tray.on('double-click', () => {
        clearTimeout(clickTimer);
        showMainWindow();
    });
    refreshTray();

    return tray;
};
