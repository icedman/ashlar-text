#ifndef ICONS_H
#define ICONS_H

#include <QPixmap>

#include "extension.h"

QPixmap icon_for_file(icon_theme_ptr icons, QString& filename, QString& suffix, std::vector<Extension>& extensions);
QPixmap icon_for_folder(icon_theme_ptr icons, QString& folder, bool open, std::vector<Extension>& extensions);

#endif // ICONS_H