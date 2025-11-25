#include "GameDetectorDock.h"
#include "GameDetector.h"
#include "TwitchChatBot.h"
#include "TwitchAuthManager.h"
#include "ConfigManager.h"
#include "GameDetectorSettingsDialog.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QFrame>
#include <QUrl>
#include <QDesktopServices>
#include <QStyle>
#include <QCheckBox>
#include <QCursor>
#include <QFormLayout>
#include <QMessageBox>
#include <QTimer>
#include <QVBoxLayout>
#include <obs.h>

GameDetectorDock::GameDetectorDock(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(5, 5, 5, 5);
	this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

	statusLabel = new QLabel(obs_module_text("Status.Waiting"));
	statusLabel->setWordWrap(true);
	mainLayout->addWidget(statusLabel); 

	QFrame *separator1 = new QFrame();
	separator1->setFrameShape(QFrame::HLine);
	separator1->setFrameShadow(QFrame::Sunken);
	mainLayout->addWidget(separator1);

	QFormLayout *executionLayout = new QFormLayout();
	executionLayout->setContentsMargins(0, 5, 0, 5);

	autoExecuteCheckbox = new QCheckBox(obs_module_text("Dock.AutoExecute"));
	executionLayout->addRow(autoExecuteCheckbox);

	// Layout horizontal para os botões de ação
	QHBoxLayout *buttonsLayout = new QHBoxLayout();
	executeCommandButton = new QPushButton(obs_module_text("Dock.SetGame"));
	buttonsLayout->addWidget(executeCommandButton);

	settingsButton = new QPushButton();
	settingsButton->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
	settingsButton->setToolTip(obs_module_text("Dock.OpenSettings"));
	settingsButton->setCursor(Qt::PointingHandCursor);
	settingsButton->setFixedSize(executeCommandButton->sizeHint().height(), executeCommandButton->sizeHint().height());
	buttonsLayout->addWidget(settingsButton);

	executionLayout->addRow(buttonsLayout);

	setJustChattingButton = new QPushButton(obs_module_text("Dock.SetJustChatting"));
	executionLayout->addRow(setJustChattingButton);

	mainLayout->addLayout(executionLayout);

	connect(executeCommandButton, &QPushButton::clicked, this,
			&GameDetectorDock::onExecuteCommandClicked);
	connect(setJustChattingButton, &QPushButton::clicked, this, &GameDetectorDock::onSetJustChattingClicked);
	connect(&GameDetector::get(), &GameDetector::gameDetected, this, &GameDetectorDock::onGameDetected);
	connect(&GameDetector::get(), &GameDetector::noGameDetected, this, &GameDetectorDock::onNoGameDetected);
	connect(&TwitchChatBot::get(), &TwitchChatBot::categoryUpdateFinished, this,
		QOverload<bool, const QString &, const QString &>::of(&GameDetectorDock::onCategoryUpdateFinished));
	connect(&TwitchChatBot::get(), &TwitchChatBot::authenticationRequired, this,
		&GameDetectorDock::onAuthenticationRequired);

	connect(autoExecuteCheckbox, &QCheckBox::checkStateChanged, this, &GameDetectorDock::onSettingsChanged);

	saveDelayTimer = new QTimer(this);
	saveDelayTimer->setSingleShot(true);
	saveDelayTimer->setInterval(1000);
	connect(saveDelayTimer, &QTimer::timeout, this, &GameDetectorDock::saveDockSettings);

	connect(&ConfigManager::get(), &ConfigManager::settingsSaved, this, &GameDetectorDock::checkWarningsAndStatus);

	statusCheckTimer = new QTimer(this);
	connect(statusCheckTimer, &QTimer::timeout, this, &GameDetectorDock::checkWarningsAndStatus);
	statusCheckTimer->start(5000);

	connect(settingsButton, &QPushButton::clicked, this, &GameDetectorDock::onSettingsButtonClicked);

	mainLayout->addStretch(1);

	setLayout(mainLayout);
}

void GameDetectorDock::saveDockSettings()
{
	obs_data_t *settings = ConfigManager::get().getSettings();

	obs_data_set_bool(settings, "execute_automatically", autoExecuteCheckbox->isChecked());

	ConfigManager::get().save(settings);

	statusLabel->setText(obs_module_text("Dock.SettingsSaved"));
	QTimer::singleShot(2000, this, &GameDetectorDock::checkWarningsAndStatus);
}

void GameDetectorDock::onSettingsChanged()
{
	saveDelayTimer->start();
}

void GameDetectorDock::onGameDetected(const QString &gameName, const QString &processName)
{
	this->detectedGameName = gameName;
	statusLabel->setText(QString(obs_module_text("Status.Playing")).arg(gameName));

	if (autoExecuteCheckbox->isChecked()) {
		int actionMode = ConfigManager::get().getTwitchActionMode();
		if (actionMode == 0) {
			executeAction(gameName);
		} else {
			TwitchChatBot::get().updateCategory(gameName);
		}
	}
}

void GameDetectorDock::onNoGameDetected()
{
	this->detectedGameName.clear();
	statusLabel->setText(obs_module_text("Status.Waiting"));

	if (autoExecuteCheckbox->isChecked()) {
		QString noGameCommand = ConfigManager::get().getNoGameCommand();
		int actionMode = ConfigManager::get().getTwitchActionMode();
		if (actionMode == 0) {
			if (!noGameCommand.isEmpty()) {
				TwitchChatBot::get().sendChatMessage(noGameCommand);
			}
		} else {
			TwitchChatBot::get().updateCategory("Just Chatting");
		}
	}
}

void GameDetectorDock::onExecuteCommandClicked()
{
	int actionMode = ConfigManager::get().getTwitchActionMode();
	if (!detectedGameName.isEmpty()) {
		if (actionMode == 0) {
			executeAction(detectedGameName);
		} else {
			TwitchChatBot::get().updateCategory(detectedGameName);
		}
	} else {
		QString noGameCommand = ConfigManager::get().getNoGameCommand();
		if (actionMode == 0) {
			TwitchChatBot::get().sendChatMessage(noGameCommand);
		} else {
			TwitchChatBot::get().updateCategory("Just Chatting");
		}
	}
}

void GameDetectorDock::onSetJustChattingClicked()
{
	int actionMode = ConfigManager::get().getTwitchActionMode();
	if (actionMode == 0) {
		QString noGameCommand = ConfigManager::get().getNoGameCommand();
		if (!noGameCommand.isEmpty())
			TwitchChatBot::get().sendChatMessage(noGameCommand);
	} else {
		TwitchChatBot::get().updateCategory("Just Chatting");
	}
}

void GameDetectorDock::loadSettingsFromConfig()
{
	autoExecuteCheckbox->blockSignals(true);

	autoExecuteCheckbox->setChecked(ConfigManager::get().getExecuteAutomatically());

	autoExecuteCheckbox->blockSignals(false);
	checkWarningsAndStatus();
}

void GameDetectorDock::executeAction(const QString &gameName)
{
	QString commandTemplate = ConfigManager::get().getCommand();
	if (commandTemplate.isEmpty()) return;

	QString command = commandTemplate.replace("{game}", gameName);
	TwitchChatBot::get().sendChatMessage(command);
}

void GameDetectorDock::onCategoryUpdateFinished(bool success, const QString &gameName, const QString &errorString)
{
	if (success) {
		statusLabel->setText(QString(obs_module_text("Dock.CategoryUpdated")).arg(gameName));
	} else {
		statusLabel->setText(QString(errorString).arg(gameName));
	}

	QTimer::singleShot(3000, this, &GameDetectorDock::restoreStatusLabel);
}

void GameDetectorDock::checkWarningsAndStatus()
{
	if (GameDetector::get().isGameListEmpty()) {
        statusLabel->setText(obs_module_text("Status.Warning.NoGames"));
        return;
	}

	bool notConnected = ConfigManager::get().getUserId().isEmpty();
	if (notConnected) {
		statusLabel->setText(obs_module_text("Status.Warning.NotConnected"));
		return;
	}

	restoreStatusLabel();
}

void GameDetectorDock::restoreStatusLabel()
{
	if (!detectedGameName.isEmpty())
		statusLabel->setText(QString(obs_module_text("Status.Playing")).arg(detectedGameName));
	else {
		statusLabel->setText(obs_module_text("Status.Waiting"));
	}
}

void GameDetectorDock::onSettingsButtonClicked()
{
	GameDetectorSettingsDialog dialog(this);
	dialog.exec();
}

void GameDetectorDock::onAuthenticationRequired()
{
	QMessageBox msgBox;
	msgBox.setWindowTitle(obs_module_text("Auth.Required.Title"));
	msgBox.setText(obs_module_text("Auth.Required.Text"));
	msgBox.setInformativeText(obs_module_text("Auth.Required.Info"));
	msgBox.setIcon(QMessageBox::Question);
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);
	msgBox.button(QMessageBox::Yes)->setText(obs_module_text("Auth.Required.Connect"));
	msgBox.button(QMessageBox::No)->setText(obs_module_text("Auth.Cancel"));
	if (msgBox.exec() == QMessageBox::Yes) {
		TwitchAuthManager::get().startAuthentication();
	}
}

GameDetectorDock::~GameDetectorDock()
{
}