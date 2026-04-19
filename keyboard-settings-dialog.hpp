#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QGroupBox>
#include <QCheckBox>
#include <obs-frontend-api.h>
#include "ui-utils.hpp"

class KeyboardSettingsDialog : public QDialog {
	Q_OBJECT

public:
	KeyboardSettingsDialog(QWidget *parent = nullptr);
	~KeyboardSettingsDialog();

	void LoadSettings();
	void SaveSettings();

	void PopulateBrowserSourceComboBox();

	// Hook management
	bool isHookEnabled() const { return hookEnabled; }
	bool enableHook();
	void disableHook();

public slots:
	void toggleHook();

private slots:
	void applySettings();
	void onDisplayInBrowserSourceToggled(bool checked);

private:
	void setupUI();

	bool hookEnabled = false;

	// UI Layouts
	QVBoxLayout *mainLayout;
	
	// UI Components
	QComboBox *browserSourceComboBox;
	QCheckBox *displayInBrowserSourceCheckBox;
	QCheckBox *hookEnabledCheckBox;
	QCheckBox *startWithOBSCheckBox;
	
	// Single key capture UI elements
	QCheckBox *captureNumpadCheckBox;
	QCheckBox *captureNumbersCheckBox;
	QCheckBox *captureLettersCheckBox;
	QCheckBox *capturePunctuationCheckBox;
	QPlainTextEdit *whitelistTextEdit;

	// Display format UI elements
	QLineEdit *separatorLineEdit;

	// Logging UI elements
	QCheckBox *enableLoggingCheckBox;
	
	QPushButton *applyButton;
	QPushButton *closeButton;

	// OBS settings storage
	obs_data_t *settings = nullptr;
};
