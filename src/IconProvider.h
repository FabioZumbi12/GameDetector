#ifndef ICONPROVIDER_H
#define ICONPROVIDER_H

#include <QIcon>
#include <QString>
#include <QFile>

class IconProvider {
public:
	static QIcon getIconForFile(const QString &filePath);
};

#endif // ICONPROVIDER_H
