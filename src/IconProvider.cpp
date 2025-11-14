#include "IconProvider.h"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <QPixmap>
#include <QPainter>

QIcon IconProvider::getIconForFile(const QString &filePath)
{
	if (filePath.isEmpty() || !QFile::exists(filePath))
		return QIcon();

	// Usa ExtractIconExW para uma extração mais confiável dos ícones embutidos.
	const std::wstring wideFilePath = filePath.toStdWString();
	UINT iconCount = ExtractIconExW(wideFilePath.c_str(), -1, nullptr, nullptr, 0);

	if (iconCount > 0) {
		std::vector<HICON> largeIcons(iconCount);
		std::vector<HICON> smallIcons(iconCount);

		UINT extractedCount = ExtractIconExW(wideFilePath.c_str(), 0, largeIcons.data(), smallIcons.data(), iconCount);

		if (extractedCount > 0) {
			// Prioriza o primeiro ícone grande, que geralmente é o de melhor qualidade.
			HICON bestIcon = largeIcons[0] ? largeIcons[0] : smallIcons[0];

			if (bestIcon) {
				QPixmap pixmap = QPixmap::fromImage(QImage::fromHICON(bestIcon));

				// Libera a memória de todos os ícones extraídos
				for (UINT i = 0; i < extractedCount; ++i) {
					if (largeIcons[i]) DestroyIcon(largeIcons[i]);
					if (smallIcons[i]) DestroyIcon(smallIcons[i]);
				}

				return QIcon(pixmap);
			}
		}
	}

	// Se ExtractIconExW falhar, retorna um ícone vazio.
	// A lógica na janela de configurações usará um ícone padrão.
	return QIcon();
}
#endif
