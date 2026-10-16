import enAccount from './locales/en/account.json';
import enAnnotate from './locales/en/annotate.json';
import enApp from './locales/en/app.json';
import enCaptureSettings from './locales/en/captureSettings.json';
import enConnect from './locales/en/connect.json';
import enDrive from './locales/en/drive.json';
import enDriveSettings from './locales/en/driveSettings.json';
import enEditor from './locales/en/editor.json';
import enGeneral from './locales/en/general.json';
import enHome from './locales/en/home.json';
import enHotkeys from './locales/en/hotkeys.json';
import enLabels from './locales/en/labels.json';
import enNav from './locales/en/nav.json';
import enOverlay from './locales/en/overlay.json';
import enRecorder from './locales/en/recorder.json';
import enRecording from './locales/en/recording.json';
import enRecordingSettings from './locales/en/recordingSettings.json';
import enSettings from './locales/en/settings.json';
import enTasks from './locales/en/tasks.json';
import enTray from './locales/en/tray.json';
import enUpdater from './locales/en/updater.json';
import enUpdates from './locales/en/updates.json';
import enUploadSettings from './locales/en/uploadSettings.json';
import trAccount from './locales/tr/account.json';
import trAnnotate from './locales/tr/annotate.json';
import trApp from './locales/tr/app.json';
import trCaptureSettings from './locales/tr/captureSettings.json';
import trConnect from './locales/tr/connect.json';
import trDrive from './locales/tr/drive.json';
import trDriveSettings from './locales/tr/driveSettings.json';
import trEditor from './locales/tr/editor.json';
import trGeneral from './locales/tr/general.json';
import trHome from './locales/tr/home.json';
import trHotkeys from './locales/tr/hotkeys.json';
import trLabels from './locales/tr/labels.json';
import trNav from './locales/tr/nav.json';
import trOverlay from './locales/tr/overlay.json';
import trRecorder from './locales/tr/recorder.json';
import trRecording from './locales/tr/recording.json';
import trRecordingSettings from './locales/tr/recordingSettings.json';
import trSettings from './locales/tr/settings.json';
import trTasks from './locales/tr/tasks.json';
import trTray from './locales/tr/tray.json';
import trUpdater from './locales/tr/updater.json';
import trUpdates from './locales/tr/updates.json';
import trUploadSettings from './locales/tr/uploadSettings.json';

export const LANGUAGES = ['en', 'tr'] as const;

export type Language = (typeof LANGUAGES)[number];

export const messages = {
    en: {
        ...enAccount,
        ...enAnnotate,
        ...enApp,
        ...enCaptureSettings,
        ...enConnect,
        ...enDrive,
        ...enDriveSettings,
        ...enEditor,
        ...enGeneral,
        ...enHome,
        ...enHotkeys,
        ...enLabels,
        ...enNav,
        ...enOverlay,
        ...enRecorder,
        ...enRecording,
        ...enRecordingSettings,
        ...enSettings,
        ...enTasks,
        ...enTray,
        ...enUpdater,
        ...enUpdates,
        ...enUploadSettings,
    },
    tr: {
        ...trAccount,
        ...trAnnotate,
        ...trApp,
        ...trCaptureSettings,
        ...trConnect,
        ...trDrive,
        ...trDriveSettings,
        ...trEditor,
        ...trGeneral,
        ...trHome,
        ...trHotkeys,
        ...trLabels,
        ...trNav,
        ...trOverlay,
        ...trRecorder,
        ...trRecording,
        ...trRecordingSettings,
        ...trSettings,
        ...trTasks,
        ...trTray,
        ...trUpdater,
        ...trUpdates,
        ...trUploadSettings,
    },
};
