#include <Arduino.h>

#include "tft_display.h"
#include "linked_list.h"

// Save data struct
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

	const OptionMode Mode;

	SettingsOption(const String& name, const String& desc, MenuGetFunc get, MenuSetFunc set, OptionMode m);
	~SettingsOption();

	static unsigned long getYPosition(unsigned int index);
	String getModeString() const;

	void drawItem(unsigned int posY);
	void drawValue(unsigned int posY);
	void drawDescription();

private:
	static const unsigned int ItemHeight = 19;
	static const unsigned int ItemStartX = 20;
	static const unsigned int ItemStartY = 45;

	String lastValue;
};

class SettingsPage {
public:
	static void drawPage(unsigned int pos);
	static void redraw(unsigned int pos);

	static void drawCursor(unsigned int pos);
	static void drawScrollIndicator();

	static void changeOption(unsigned int pos);

	static String getButtonText(unsigned int pos);

private:
	static const unsigned int ItemsPerPage = 9;  // should be calculated given font size and screen space, but good enough for testing
	static unsigned int startingItem;

	static void drawItems();

	enum class ScrollType {
		Smooth,  // at the end of current page, moves all items up by one
		Paged,   // at the end of the current page, draws an entire new page of items
	};
	static const ScrollType Scroll = ScrollType::Paged;

	static bool updateScroll(unsigned int pos);

	static unsigned int lastItem() { return startingItem + ItemsPerPage - 1; }  // indexed at 0
	static bool onPage(unsigned int item) { return item >= startingItem && item <= lastItem(); }
	
};


extern Settings set;

void DrawScrollIndicator(float, uint32_t);
void DrawPageIndicator(unsigned int, unsigned int, uint32_t);

void println_Center(Adafruit_ILI9341& d, String heading, int centerX, int centerY);
