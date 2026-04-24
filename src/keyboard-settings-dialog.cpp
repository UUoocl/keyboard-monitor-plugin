#include "keyboard-settings-dialog.hpp"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QStyle>
#include "keyboard-monitor.hpp"

KeyboardSettingsDialog::KeyboardSettingsDialog(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(obs_module_text("Settings.Title"));
	setupUI();
	LoadSettings();
}

KeyboardSettingsDialog::~KeyboardSettingsDialog()
{
	if (settings) {
		obs_data_release(settings);
	}
}

void KeyboardSettingsDialog::setupUI()
{
	mainLayout = new QVBoxLayout(this);

	// --- Keyboard Monitoring Group ---
	QGroupBox *hookGroupBox = new QGroupBox(obs_module_text("Settings.HookGroup"), this);
	QVBoxLayout *hookLayout = new QVBoxLayout(hookGroupBox);

	hookEnabledCheckBox = new QCheckBox(obs_module_text("Settings.HookEnabled"), this);
	startWithOBSCheckBox = new QCheckBox(obs_module_text("Settings.StartWithOBS"), this);
	
	hookLayout->addWidget(hookEnabledCheckBox);
	hookLayout->addWidget(startWithOBSCheckBox);
	
	mainLayout->addWidget(hookGroupBox);

	mainLayout->addWidget(hookGroupBox);

	// --- Key Capture Settings ---
	QGroupBox *captureGroupBox = new QGroupBox(obs_module_text("Settings.CaptureGroup"), this);
	QVBoxLayout *captureLayout = new QVBoxLayout(captureGroupBox);

	captureNumpadCheckBox = new QCheckBox(obs_module_text("Settings.CaptureNumpad"), this);
	captureNumbersCheckBox = new QCheckBox(obs_module_text("Settings.CaptureNumbers"), this);
	captureLettersCheckBox = new QCheckBox(obs_module_text("Settings.CaptureLetters"), this);
	capturePunctuationCheckBox = new QCheckBox(obs_module_text("Settings.CapturePunctuation"), this);

	captureLayout->addWidget(captureNumpadCheckBox);
	captureLayout->addWidget(captureNumbersCheckBox);
	captureLayout->addWidget(captureLettersCheckBox);
	captureLayout->addWidget(capturePunctuationCheckBox);

	captureLayout->addWidget(new QLabel(obs_module_text("Settings.Whitelist"), this));

	whitelistTextEdit = new QPlainTextEdit(this);
	whitelistTextEdit->setPlaceholderText("e.g. A, B, C, 1, 2, 3");
	whitelistTextEdit->setFixedHeight(60);
	captureLayout->addWidget(whitelistTextEdit);

	mainLayout->addWidget(captureGroupBox);

	// --- Format & Misc ---
	QGroupBox *miscGroupBox = new QGroupBox(obs_module_text("Settings.MiscGroup"), this);
	QVBoxLayout *miscLayout = new QVBoxLayout(miscGroupBox);

	miscLayout->addWidget(new QLabel(obs_module_text("Settings.Separator"), this));

	separatorLineEdit = new QLineEdit(this);
	miscLayout->addWidget(separatorLineEdit);

	enableLoggingCheckBox = new QCheckBox(obs_module_text("Settings.EnableLogging"), this);
	miscLayout->addWidget(enableLoggingCheckBox);

	mainLayout->addWidget(miscGroupBox);
	mainLayout->addStretch();

	// --- Footer Buttons ---
	QHBoxLayout *footerBtnLayout = new QHBoxLayout();
	applyButton = new QPushButton(obs_module_text("Settings.Apply"), this);
	closeButton = new QPushButton(obs_module_text("Settings.Close"), this);

	footerBtnLayout->addStretch();
	footerBtnLayout->addWidget(closeButton);
	footerBtnLayout->addWidget(applyButton);
	mainLayout->addLayout(footerBtnLayout);

	// Connect signals
	connect(applyButton, &QPushButton::clicked, this, &KeyboardSettingsDialog::applySettings);
	connect(closeButton, &QPushButton::clicked, this, &QDialog::hide);
	connect(hookEnabledCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
		if (checked) enableHook(); else disableHook();
	});
}


void KeyboardSettingsDialog::LoadSettings()
{
	if (settings) obs_data_release(settings);
	settings = SaveLoadSettingsCallback(nullptr, false);

	if (settings) {
		// Block signals to prevent toggled() events from triggering SaveSettings prematurely
		const bool oldState = hookEnabledCheckBox->blockSignals(true);
		
		hookEnabled = IsMonitoringEnabled();
		hookEnabledCheckBox->setChecked(hookEnabled);
		startWithOBSCheckBox->setChecked(obs_data_get_bool(settings, "startWithOBS"));

		startWithOBSCheckBox->setChecked(obs_data_get_bool(settings, "startWithOBS"));

		captureNumpadCheckBox->setChecked(obs_data_get_bool(settings, "captureNumpad"));
		captureNumbersCheckBox->setChecked(obs_data_get_bool(settings, "captureNumbers"));
		captureLettersCheckBox->setChecked(obs_data_get_bool(settings, "captureLetters"));
		capturePunctuationCheckBox->setChecked(obs_data_get_bool(settings, "capturePunctuation"));
		
		whitelistTextEdit->setPlainText(QString::fromUtf8(obs_data_get_string(settings, "whitelistedKeys")));
		separatorLineEdit->setText(QString::fromUtf8(obs_data_get_string(settings, "keySeparator")));
		enableLoggingCheckBox->setChecked(obs_data_get_bool(settings, "enableLogging"));

		hookEnabledCheckBox->blockSignals(oldState);
	}
}

void KeyboardSettingsDialog::SaveSettings()
{
	if (!settings) settings = obs_data_create();

	obs_data_set_bool(settings, "hookEnabled", hookEnabled);
	obs_data_set_bool(settings, "startWithOBS", startWithOBSCheckBox->isChecked());
	
	obs_data_set_bool(settings, "captureNumpad", captureNumpadCheckBox->isChecked());
	obs_data_set_bool(settings, "captureNumbers", captureNumbersCheckBox->isChecked());
	obs_data_set_bool(settings, "captureLetters", captureLettersCheckBox->isChecked());
	obs_data_set_bool(settings, "capturePunctuation", capturePunctuationCheckBox->isChecked());
	
	obs_data_set_string(settings, "whitelistedKeys", whitelistTextEdit->toPlainText().toUtf8().constData());
	obs_data_set_string(settings, "keySeparator", separatorLineEdit->text().toUtf8().constData());
	obs_data_set_bool(settings, "enableLogging", enableLoggingCheckBox->isChecked());

	SaveLoadSettingsCallback(settings, true);
	loadSingleKeyCaptureSettings(settings);
}

void KeyboardSettingsDialog::applySettings()
{
	SaveSettings();
}

void KeyboardSettingsDialog::toggleHook()
{
	if (hookEnabled) disableHook(); else enableHook();
}

bool KeyboardSettingsDialog::enableHook()
{
	hookEnabled = EnableMonitoring();
	hookEnabledCheckBox->setChecked(hookEnabled);
	SaveSettings();
	return hookEnabled;
}

void KeyboardSettingsDialog::disableHook()
{
	DisableMonitoring();
	hookEnabled = false;
	hookEnabledCheckBox->setChecked(false);
	SaveSettings();
}

