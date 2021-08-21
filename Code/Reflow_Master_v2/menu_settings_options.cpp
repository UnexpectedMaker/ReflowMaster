#include "menu_settings.h"

void ShowPaste();
void ShowResetDefaults();

using OptionMode = SettingsOption::OptionMode;

// "Switch Paste" menu option
String getSwitchPaste() {
	return "";
}

void changeSwitchPaste() {
	ShowPaste();
}

SettingsOption OptionPasteMenu("SWITCH PASTE", "Select which profile to reflow", getSwitchPaste, changeSwitchPaste, OptionMode::Select);


// "Use Fan" menu option
String getUseFan() {
	return set.useFan ? "ON" : "OFF";
}

void changeUseFan() {
	set.useFan = !set.useFan;
}

SettingsOption OptionUseFan("USE FAN", "Enable fan for end of reflow, requires 5V DC fan", getUseFan, changeUseFan, OptionMode::Change);


// "Fan Countdown" menu option
String getFanCountdown() {
	return String(set.fanTimeAfterReflow) + "s";
}

void changeFanCountdown() {
	set.fanTimeAfterReflow += 5;
	if (set.fanTimeAfterReflow > 60)
		set.fanTimeAfterReflow = 0;
}

SettingsOption OptionFanCountdown("FAN COUNTDOWN", "Keep fan on for XXX sec after reflow", getFanCountdown, changeFanCountdown, OptionMode::Change);


// "Graph Look Ahead" menu option
String getGraphLookAhead() {
	return String(set.lookAhead);
}

void changeGraphLookAhead() {
	set.lookAhead += 1;
	if (set.lookAhead > 15)
		set.lookAhead = 1;
}

SettingsOption OptionGraphLookAhead("GRAPH LOOK AHEAD", "Soak and Reflow look ahead for rate change speed", getGraphLookAhead, changeGraphLookAhead, OptionMode::Change);


// "Power" menu option
String getPower() {
	return String(round((set.power * 100))) + "%";
}

void changePower() {
	set.power += 0.1;
	if (set.power > 1.55)
		set.power = 0.5;
}

SettingsOption OptionPower("POWER", "Adjust the power boost", getPower, changePower, OptionMode::Change);


// "Temp Offset" menu option
String getTempOffset() {
	return String(set.tempOffset);
}

void changeTempOffset() {
	set.tempOffset += 1;
	if (set.tempOffset > 15)
		set.tempOffset = -15;
}

SettingsOption OptionTempOffset("TEMP OFFSET", "Adjust temp probe reading offset", getTempOffset, changeTempOffset, OptionMode::Change);


// "Start Ramp 100%" menu option
String getStartFullBlast() {
	return set.startFullBlast ? "ON" : "OFF";
}

void changeStartFullBlast() {
	set.startFullBlast = !set.startFullBlast;
}

SettingsOption OptionStartFullBlast("START RAMP 100%", "Force full power on initial ramp-up - be careful!", getStartFullBlast, changeStartFullBlast, OptionMode::Change);


// "Bake Temp Gap" menu option
String getBakeTempGap() {
	return String(set.bakeTempGap);
}

void changeBakeTempGap() {
	set.bakeTempGap += 1;
	if (set.bakeTempGap > 5)
		set.bakeTempGap = 0;
}

SettingsOption OptionBakeTempGap("BAKE TEMP GAP", "Bake thermal mass adjustment, higher for more mass", getBakeTempGap, changeBakeTempGap, OptionMode::Change);


// "Key Tone" menu option
String getKeyToneSetting() {
	return set.keyTone ? "ON" : "OFF";
}

void setKeyToneSetting() {
	set.keyTone = !set.keyTone;
}

SettingsOption OptionKeyTone("KEY TONE", "Make a noise whenever a button is pressed", getKeyToneSetting, setKeyToneSetting, OptionMode::Change);


// "Disable Buzzer" menu option
String getBuzzerSetting() {
	return set.beep ? "OFF" : "ON";  // inverted, as the setting is to 'disable'
}

void setBuzzerSetting() {
	set.beep = !set.beep;
}

SettingsOption OptionBuzzer("DISABLE BUZZER", "Disable buzzer noise in ALL modes", getBuzzerSetting, setBuzzerSetting, OptionMode::Change);


// ############################################################################

// "Reset to defaults" menu option
String getResetToDefaults() {
	return "";
}

void goToResetDefaults() {
	ShowResetDefaults();
}

SettingsOption OptionResetToDefaults("RESET TO DEFAULTS", "Reset to default settings", getResetToDefaults, goToResetDefaults, OptionMode::Select);
