#include <Arduino.h>

#include "tft_display.h"
#include "linked_list.h"

// Forward-declare functions used in the menu
int constrainLoop(int value, int min, int max);

void ExitSettings();

void DrawScrollIndicator(float, uint32_t);
void DrawPageIndicator(unsigned int, unsigned int, uint32_t);

void ShowButtonOptions(bool clearAll);

void println_Center(Adafruit_ILI9341& d, String heading, int centerX, int centerY);

// define settings data struct
typedef struct {
	boolean valid = true;
	boolean useFan = false;
	int fanTimeAfterReflow = 60;
	byte paste = 0;
	float power = 1;
	int lookAhead = 7;
	int lookAheadWarm = 7;
	int tempOffset = 0;
	long bakeTime = 1200; // 20 mins
	float bakeTemp = 45; // Degrees C
	int bakeTempGap = 3; // Aim for the desired temp minus this value to compensate for overrun
	bool startFullBlast = false;
	bool beep = true;
	bool keyTone = true;
} Settings;


class SettingsOption : public LinkedList<SettingsOption> {
public:
	enum class OptionMode {
		Select,
		Change,
	};

	friend LinkedList<SettingsOption>;

	typedef String(*MenuGetFunc)();
	typedef void (*MenuSetFunc)();

	SettingsOption(const String& name, const String& desc, MenuGetFunc get, MenuSetFunc set, OptionMode m);
	~SettingsOption();

	bool changeValue();  // returns 'true' if we need to redraw

	static unsigned long getYPosition(unsigned int index);
	String getModeString() const;

	void drawItem(unsigned int posY);
	void drawValue(unsigned int posY);
	void drawDescription();

private:
	static const unsigned int ItemHeight = 19;
	static const unsigned int ItemStartX = 20;
	static const unsigned int ItemStartY = 45;

	const OptionMode Mode;

	const String ItemName;
	const String ItemDescription;

	const MenuGetFunc getFunction;
	const MenuSetFunc setFunction;

	String lastValue;
};

namespace SettingsPage {
	void pressButton(unsigned int num);  // record a button press on the settings page

	void drawPage(bool resetSelection = true);  // draw the Settings page in its entirety
	String getButtonText();  // get the string of text to describe button 0
};


extern Settings set;
