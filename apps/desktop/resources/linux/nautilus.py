import json
import os
import subprocess

from gi.repository import GObject, Nautilus

CONFIG = os.path.join(os.environ.get('XDG_CONFIG_HOME') or os.path.expanduser('~/.config'), 'fileship', 'menus.json')


class FileshipMenu(GObject.GObject, Nautilus.MenuProvider):
    def run(self, _item, arguments):
        subprocess.Popen(arguments)

    def item(self, name, label, arguments):
        entry = Nautilus.MenuItem(name='Fileship::' + name, label=label)
        entry.connect('activate', self.run, arguments)
        return entry

    def get_file_items(self, *arguments):
        files = arguments[-1]

        try:
            with open(CONFIG, encoding='utf-8') as file:
                config = json.load(file)
        except (OSError, ValueError):
            return []

        host = config['host']
        root = config['root']
        paths = [file.get_location().get_path() for file in files]
        if not os.path.exists(host) or not paths or None in paths:
            return []

        folders = [file.is_directory() for file in files]
        inside = [bool(root) and path.startswith(root + '/') for path in paths]

        if not any(inside):
            return [] if any(folders) else [self.item('upload', config['upload'], [host, '--upload'] + paths)]
        if not all(inside):
            return []

        return [
            self.item(verb['action'], verb['label'], [host, '--harbor-action', verb['action']] + paths)
            for verb in config['items']
            if all(folder == verb['folders'] for folder in folders) and (verb['many'] or len(paths) == 1)
        ]
