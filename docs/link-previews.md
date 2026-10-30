# Link previews

When you share a Fileship link in Discord, Slack, X or another app, it shows a preview card. Set it up in **Settings → Sharing → Embeds**.

## Embeds

Turn on **Embeds** to give your links a title, a description, a site name and a color. They apply to file pages (`/view/…`), and the title and color also to public notes and folders. While embeds are on, uploads return the file page instead of the raw file, so the preview is what people see.

The title, description and site name can use these placeholders:

| Placeholder   | Filled with                     |
| ------------- | ------------------------------- |
| `{fileName}`  | The file's link name            |
| `{mimeType}`  | Its type, for example image/png |
| `{size}`      | Its size                        |
| `{createdAt}` | When it was uploaded            |
| `{now}`       | When the preview was loaded     |

## Discord layout

Discord can show a richer preview built from components: text, images and videos in a gallery, a thumbnail next to text, separators and link buttons. With **Discord layout** on (the default while embeds are on), Fileship adds such a layout to file, note and folder pages. Other apps keep the regular embed.

**Customize layouts** opens a builder with one layout for files, one for notes and one for folders. Add components, move them up or down, and see a preview next to them. **Reset to default** brings back the built-in layout, which uses your embed's title, description and site name and the language of your account.

### Components

| Component           | What it shows                                                      |
| ------------------- | ------------------------------------------------------------------ |
| Text                | Discord Markdown: `#` headings, `-#` small text, `**bold**`, links |
| Text with side item | Up to three texts with a thumbnail or a button on the side         |
| Media gallery       | Up to ten images or videos                                         |
| Separator           | A line or a gap, small or large                                    |
| Buttons             | Up to five buttons that open a link                                |

The whole preview can be hidden behind a spoiler, and so can each image.

### Placeholders

Every layout can use `{url}` (the page's link), `{siteName}`, `{createdAt}` and `{now}`. On top of that:

| Layout | Placeholders                                                                                                                                                                                         |
| ------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| File   | `{title}` and `{description}` (your embed's, filled in), `{name}` (the uploaded name), `{fileName}`, `{mimeType}`, `{size}`, `{fileUrl}` (images and videos only), `{downloadUrl}`, `{thumbnailUrl}` |
| Note   | `{title}`, `{excerpt}` (the start of the note as plain text)                                                                                                                                         |
| Folder | `{name}`, `{files}` and `{folders}` (counted, in your language), `{fileCount}`, `{folderCount}`                                                                                                      |

Placeholders that are empty for a link are left out: a gallery with only `{fileUrl}` disappears for a text file, a side thumbnail without an image turns into plain text, and a line that only held an empty placeholder is removed.

### Limits

- Discord shows images and videos only. Buttons and media need links that start with `http://` or `https://`; others are left out.
- A layout holds up to 40 components, buttons and texts inside other components included.
- A layout that ends up empty or too large for Discord cannot be saved, and a link whose layout ends up empty gets the regular embed.
- Password protected files (unless the link carries the password) and private notes and folders never get a Discord layout.
- Discord keeps a preview for about 30 minutes. To see a change right away, add something to the link, such as `?v=2`.
- Discord has to reach your Fileship to read the page, so previews only work on a public address.
