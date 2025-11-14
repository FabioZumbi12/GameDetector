#include "GameDetectorDock.h"
#include "GameDetector.h"
#include <obs-data.h>
#include "ConfigManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QUrl>
#include <QDesktopServices>
#include <QTimer>
#include <obs.h>
#include <QHeaderView>
#include <QStyle>
#include <QCheckBox>

static QLabel *g_statusLabel = nullptr;

GameDetectorDock::GameDetectorDock(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(10, 10, 10, 10);
	mainLayout->setSpacing(10);
	this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

	g_statusLabel = new QLabel(obs_module_text("Status.Waiting"));
	mainLayout->addWidget(g_statusLabel);

	// Layout para o comando
	QHBoxLayout *commandLayout = new QHBoxLayout();
	commandLabel = new QLabel(obs_module_text("Dock.Command.GameDetected"));

	commandInput = new QLineEdit();
	commandInput->setPlaceholderText(obs_module_text("Dock.Command.GameDetected.Placeholder"));

	commandLayout->addWidget(commandLabel);
	commandLayout->addWidget(commandInput);
	mainLayout->addLayout(commandLayout);

	QHBoxLayout *noGameCommandLayout = new QHBoxLayout();
	noGameCommandLabel = new QLabel(obs_module_text("Dock.Command.NoGame"));

	noGameCommandInput = new QLineEdit();
	noGameCommandInput->setPlaceholderText(obs_module_text("Dock.Command.NoGame.Placeholder"));

	noGameCommandLayout->addWidget(noGameCommandLabel);
	noGameCommandLayout->addWidget(noGameCommandInput);
	mainLayout->addLayout(noGameCommandLayout);

	autoExecuteCheckbox = new QCheckBox(obs_module_text("Dock.AutoExecute"));
	mainLayout->addWidget(autoExecuteCheckbox);

	executeCommandButton = new QPushButton(obs_module_text("Dock.ExecuteCommand"));
	executeCommandButton->setEnabled(false);
	mainLayout->addWidget(executeCommandButton);
	connect(executeCommandButton, &QPushButton::clicked, this,
		&GameDetectorDock::onExecuteCommandClicked);

	// Conecta os sinais de mudança de texto ao nosso novo slot
	connect(commandInput, &QLineEdit::textChanged, this, &GameDetectorDock::onSettingsChanged);
	connect(noGameCommandInput, &QLineEdit::textChanged, this, &GameDetectorDock::onSettingsChanged);
	connect(autoExecuteCheckbox, &QCheckBox::checkStateChanged, this, &GameDetectorDock::onSettingsChanged);

	// Conecta os sinais do detector de jogos aos nossos novos slots
	connect(&GameDetector::get(), &GameDetector::gameDetected, this, &GameDetectorDock::onGameDetected);
	connect(&GameDetector::get(), &GameDetector::noGameDetected, this, &GameDetectorDock::onNoGameDetected);

	// Botão de salvar no final
	saveButton = new QPushButton(obs_module_text("Dock.SaveSettings"));
	saveButton->setEnabled(false);
	mainLayout->addWidget(saveButton);
	connect(saveButton, &QPushButton::clicked, this, &GameDetectorDock::onSaveClicked);

	mainLayout->addStretch(1);
	setLayout(mainLayout);
}

void GameDetectorDock::onSaveClicked()
{
	obs_data_t *settings = ConfigManager::get().getSettings();

	// Salva apenas os comandos e o checkbox
	obs_data_set_string(settings, "twitch_command_message", commandInput->text().toStdString().c_str());
	obs_data_set_string(settings, "twitch_command_no_game", noGameCommandInput->text().toStdString().c_str());
	obs_data_set_bool(settings, "execute_automatically", autoExecuteCheckbox->isChecked());

	ConfigManager::get().save(settings);

	saveButton->setText(obs_module_text("Dock.SettingsSaved"));
	saveButton->setEnabled(false);

	QTimer::singleShot(2000, this, [this]() {
		saveButton->setText(obs_module_text("Dock.SaveSettings"));
	});
}

void GameDetectorDock::onSettingsChanged()
{
	saveButton->setEnabled(true);
	saveButton->setText(obs_module_text("Dock.SaveChanges"));
	// A linha que salvava automaticamente foi removida daqui.
}

void GameDetectorDock::onGameDetected(const QString &gameName, const QString &processName)
{
	this->detectedGameName = gameName;
	g_statusLabel->setText(QString(obs_module_text("Status.Playing")).arg(gameName));

	if (autoExecuteCheckbox->isChecked()) {
		executeGameCommand(gameName);
	} else {
		executeCommandButton->setEnabled(true);
		executeCommandButton->setText(QString(obs_module_text("Dock.ExecuteCommandFor")).arg(gameName));
	}
}

void GameDetectorDock::onNoGameDetected()
{
	this->detectedGameName.clear();
	g_statusLabel->setText(obs_module_text("Status.Waiting"));
	executeCommandButton->setEnabled(false);
	executeCommandButton->setText(obs_module_text("Dock.ExecuteCommand"));

	// Executa o comando de "sem jogo"
	QString noGameCommand = noGameCommandInput->text();
	if (!noGameCommand.isEmpty()) {
		blog(LOG_INFO, "[OBSGameDetector] Executando comando 'sem jogo': %s", noGameCommand.toStdString().c_str());
		// Aqui virá a lógica para enviar a mensagem para a Twitch
	}
}

void GameDetectorDock::onExecuteCommandClicked()
{
	if (!detectedGameName.isEmpty())
		executeGameCommand(detectedGameName);
}

void GameDetectorDock::loadSettingsFromConfig()
{
	// Bloqueia sinais para não disparar onSettingsChanged durante o carregamento
	commandInput->blockSignals(true);
	noGameCommandInput->blockSignals(true);
	autoExecuteCheckbox->blockSignals(true);

	commandInput->setText(ConfigManager::get().getCommand());
	noGameCommandInput->setText(ConfigManager::get().getNoGameCommand());
	autoExecuteCheckbox->setChecked(ConfigManager::get().getExecuteAutomatically());

	saveButton->setEnabled(false);
	saveButton->setText(obs_module_text("Dock.SaveSettings"));

	commandInput->blockSignals(false);
	noGameCommandInput->blockSignals(false);
	autoExecuteCheckbox->blockSignals(false);
}

void GameDetectorDock::executeGameCommand(const QString &gameName)
{
	QString commandTemplate = commandInput->text();
	if (commandTemplate.isEmpty()) return;

	QString command = commandTemplate.replace("{game}", gameName);
	blog(LOG_INFO, "[OBSGameDetector] Executando comando: %s", command.toStdString().c_str());
	// Aqui virá a lógica para enviar a mensagem para a Twitch
}

GameDetectorDock::~GameDetectorDock()
{
	// O Qt parent/child system geralmente cuida da limpeza.
}