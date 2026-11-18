#pragma once

#include <optional>
#include <string>

namespace fileship::ui {

double caretBlinkMs();
std::optional<std::wstring> clipboardText();
void setClipboardText(std::wstring const& text);

}
