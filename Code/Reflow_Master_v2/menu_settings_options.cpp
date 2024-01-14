#include "menu_settings.h"

void ShowPaste();
void ShowResetDefaults();

// "Switch Paste" menu option
SettingsOptionLink OptionPasteMenu("SWITCH PASTE", "Select which profile to reflow", ShowPaste);


// "Use Fan" menu option
String getUseFan() {
	return set.useFan ? "ON" : "OFF";
}

void changeUseFan() {
	set.useFan = !set.useFan;
}

SettingsOptionToggle OptionUseFan("USE FAN", "Enable fan for end of reflow, requires 5V DC fan", getUseFan, changeUseFan);


// "Fan Countdown" menu option
String getFanCountdown() {
	return String(set.fanTimeAfterReflow) + "s";
}

void changeFanCountdown() {
	set.fanTimeAfterReflow += 5;
	if (set.fanTimeAfterReflow > 60)
		set.fanTimeAfterReflow = 0;
}

SettingsOptionAdjust OptionFanCountdown("FAN COUNTDOWN", "Keep fan on for XXX sec after reflow", getFanCountdown, changeFanCountdown);


// "Graph Look Ahead" menu option
String getGraphLookAhead() {
	return String(set.lookAhead);
}

void changeGraphLookAhead() {
	set.lookAhead += 1;
	if (set.lookAhead > 15)
		set.lookAhead = 1;
}

SettingsOptionAdjust OptionGraphLookAhead("GRAPH LOOK AHEAD", "Soak and Reflow look ahead for rate change speed", getGraphLookAhead, changeGraphLookAhead);


// "Power" menu option
String getPower() {
	return String(round((set.power * 100))) + "%";
}

void changePower() {
	set.power += 0.1;
	if (set.power > 1.55)
		set.power = 0.5;
}

SettingsOptionAdjust OptionPower("POWER", "Adjust the power boost", getPower, changePower);


// "Temp Offset" menu option
String getTempOffset() {
	return String(set.tempOffset);
}

void changeTempOffset() {
	set.tempOffset += 1;
	if (set.tempOffset > 15)
		set.tempOffset = -15;
}

SettingsOptionAdjust OptionTempOffset("TEMP OFFSET", "Adjust temp probe reading offset", getTempOffset, changeTempOffset);


// "Start Ramp 100%" menu option
String getStartFullBlast() {
	return set.startFullBlast ? "ON" : "OFF";
}

void changeStartFullBlast() {
	set.startFullBlast = !set.startFullBlast;
}

SettingsOptionToggle OptionStartFullBlast("START RAMP 100%", "Force full power on initial ramp-up - be careful!", getStartFullBlast, changeStartFullBlast);


// "Bake Temp Gap" menu option
String getBakeTempGap() {
	return String(set.bakeTempGap);
}

void changeBakeTempGap() {
	set.bakeTempGap += 1;
	if (set.bakeTempGap > 5)
		set.bakeTempGap = 0;
}

SettingsOptionAdjust OptionBakeTempGap("BAKE TEMP GAP", "Bake thermal mass adjustment, higher for more mass", getBakeTempGap, changeBakeTempGap);


// "Key Tone" menu option
String getKeyToneSetting() {
	return set.keyTone ? "ON" : "OFF";
}

void setKeyToneSetting() {
	set.keyTone = !set.keyTone;
}

SettingsOptionToggle OptionKeyTone("KEY TONE", "Make a noise whenever a button is pressed", getKeyToneSetting, setKeyToneSetting);


// "Disable Buzzer" menu option
String getBuzzerSetting() {
	return set.beep ? "OFF" : "ON";  // inverted, as the setting is to 'disable'
}

void setBuzzerSetting() {
	set.beep = !set.beep;
}

SettingsOptionToggle OptionBuzzer("DISABLE BUZZER", "Disable buzzer noise in ALL modes", getBuzzerSetting, setBuzzerSetting);


// "Startup Tone" menu option
String getStartupTuneSetting() {
	return set.startupTune ? "ON" : "OFF";
}

void setStartupTuneSetting() {
	set.startupTune = !set.startupTune;
}

SettingsOptionToggle OptionStartupTune("STARTUP TUNE", "Play a little song on startup", getStartupTuneSetting, setStartupTuneSetting);


// ############################################################################

// "Reset to defaults" menu option
SettingsOptionLink OptionResetToDefaults("RESET TO DEFAULTS", "Reset to default settings", ShowResetDefaults);
