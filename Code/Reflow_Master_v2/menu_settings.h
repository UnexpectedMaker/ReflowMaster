#include <Arduino.h>
#include <Adafruit_ILI9341.h>  // for TFT drawing

#include "linked_list.h"

// Save data struct
typedef struct {
	boolean valid = false;
	boolean useFan = false;
	int fanTimeAfterReflow = 60;
	byte paste = 0;
	float power = 1;
	int lookAhead = 6;
	int lookAheadWarm = 1;
	int tempOffset = 0;
	long bakeTime = 1200; // 20 mins
	float bakeTemp = 45; // Degrees C
	int bakeTempGap = 3; // Aim for the desired temp minus this value to compensate for overrun
	bool startFullBlast = false;
	bool beep = true;
} Settings;


enum class OptionMode {
	Select,
	Change,
};

class SettingsOption : public LinkedList<SettingsOption> {
public:
	friend LinkedList<SettingsOption>;

	typedef String(*MenuGetFunc)();
	typedef void (*MenuSetFunc)();

	const String ItemName;
	const String ItemDescription;

	const MenuGetFunc getFunction;
	const MenuSetFunc setFunction;

	const bool RefreshOnChange;
	const OptionMode Mode;

	SettingsOption(const String& name, const String& desc, MenuGetFunc get, MenuSetFunc set, bool refresh, OptionMode m);
	~SettingsOption();

	static unsigned long getYPosition(unsigned int index);

	void drawItem(unsigned int posY);
	void drawDescription();

private:
	static const unsigned int ItemHeight = 19;
	static const unsigned int ItemStartY = 45;
};

class SettingsPage {
public:
	static void drawPage();
	static void redraw();

	static void drawCursor(unsigned int pos);
	static void changeOption(unsigned int pos);

private:
	static const unsigned int ItemsPerPage = 8;  // should be calculated given font size and screen space, but good enough for testing
	static unsigned int startingItem;

	static void drawItems();
};


extern Settings set;
extern Adafruit_ILI9341 tft;

void println_Center(Adafruit_ILI9341& d, String heading, int centerX, int centerY);
