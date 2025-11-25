#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>
#include <QObject>
#include <obs-module.h>

#include "ConfigManager.h"
#include "GameDetector.h"

#include "GameDetectorSettingsDialog.h"
#include "GameDetectorDock.h"

static GameDetectorDock *g_dock_widget = nullptr;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("GameDetector", "en-US")

static void open_settings_dialog(void *private_data)
{
	Q_UNUSED(private_data);
	GameDetectorSettingsDialog dialog(static_cast<QWidget *>(obs_frontend_get_main_window()));
	dialog.exec();
}

static GameDetectorDock* get_dock()
{
	return g_dock_widget;
}

bool obs_module_load(void)
{
	blog(LOG_INFO, "[GameDetector] Plugin loaded.");

	GameDetectorDock *dockWidget = new GameDetectorDock();
	obs_frontend_add_dock_by_id("game_detector", "Game Detector", dockWidget);
	g_dock_widget = dockWidget;
	blog(LOG_INFO, "[GameDetector] Dock registered.");

	obs_frontend_add_tools_menu_item(obs_module_text("Settings.ToolsMenu"), open_settings_dialog, nullptr);
	blog(LOG_INFO, "[GameDetector] Tools menu item added.");

	ConfigManager::get().load();
	get_dock()->loadSettingsFromConfig();
	blog(LOG_INFO, "[GameDetector] Config file path: %s", obs_module_config_path("config.json"));

	GameDetector::get().loadGamesFromConfig();
	GameDetector::get().startScanning();
	GameDetector::get().setupPeriodicScan();
	if (ConfigManager::get().getScanOnStartup()) {
		blog(LOG_INFO, "[GameDetector] Performing scan on startup.");
		bool scanSteam = ConfigManager::get().getScanSteam();
		bool scanEpic = ConfigManager::get().getScanEpic();
		bool scanGog = ConfigManager::get().getScanGog();
		bool scanUbisoft = ConfigManager::get().getScanUbisoft();
		GameDetector::get().rescanForGames(scanSteam, scanEpic, scanGog, scanUbisoft);
		auto conn = std::make_shared<QMetaObject::Connection>();
		*conn = QObject::connect(&GameDetector::get(), &GameDetector::automaticScanFinished,
				[conn](const QList<std::tuple<QString, QString, QString>> &foundGames) {
					GameDetector::get().mergeAndSaveGames(foundGames);
					GameDetector::get().loadGamesFromConfig(); // Recarrega a lista para o monitoramento
					QObject::disconnect(*conn); // Auto-desconexão
				});
	}

	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "[GameDetector] Plugin unloaded.");

	GameDetector::get().stopScanning();

	obs_data_t *settings = ConfigManager::get().getSettings();
	if (settings) {
		ConfigManager::get().save(settings);
		obs_data_release(settings);
	}
}