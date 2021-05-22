#include "menu_settings.h"

// Just a bunch of re-defined colours
#define BLUE      0x001F
#define TEAL      0x0438
#define GREEN     0x07E0
#define CYAN      0x07FF
#define RED       0xF800
#define MAGENTA   0xF81F
#define YELLOW    0xFFE0
#define ORANGE    0xFC00
#define PINK      0xF81F
#define PURPLE    0x8010
#define GREY      0xC618
#define WHITE     0xFFFF
#define BLACK     0x0000
#define DKBLUE    0x000D
#define DKTEAL    0x020C
#define DKGREEN   0x03E0
#define DKCYAN    0x03EF
#define DKRED     0x6000
#define DKMAGENTA 0x8008
#define DKYELLOW  0x8400
#define DKORANGE  0x8200
#define DKPINK    0x9009
#define DKPURPLE  0x4010
#define DKGREY    0x4A49


template<>
SettingsOption* LinkedList<SettingsOption>::head = nullptr;

unsigned int SettingsPage::startingItem = 0;


SettingsOption::SettingsOption(const String& name, const String& desc, MenuGetFunc get, MenuSetFunc set, bool refresh, OptionMode m)
	:
	ItemName(name), ItemDescription(desc),
	getFunction(get), setFunction(set),
	RefreshOnChange(refresh),
	Mode(m)
{
	LinkedList<SettingsOption>::add(this);
}

SettingsOption::~SettingsOption()
{
	LinkedList<SettingsOption>::remove(this);
}

unsigned long SettingsOption::getYPosition(unsigned int index) {
	return (ItemHeight * index) + ItemStartY;  // y coordinate on center
}

void SettingsOption::drawItem(unsigned int position) {
	const unsigned int yPos = getYPosition(position);
	tft.fillRect(15, yPos - 5, 240, ItemHeight, BLACK);
	tft.setCursor(20, yPos);

	tft.setTextSize(2);
	tft.setTextColor(WHITE, BLACK);
	tft.print(ItemName);
	tft.print(' ');

	tft.setTextColor(YELLOW, BLACK);
	tft.print(getFunction());
}

void SettingsOption::drawDescription() {
	const unsigned int height_area = 20;
	const unsigned int height_text = 16;

	tft.setTextSize(1);
	tft.setTextColor(GREEN, BLACK);
	tft.fillRect(0, tft.height() - height_area, tft.width(), height_area, BLACK);

	int textPosY = tft.height() - height_text;
	println_Center(tft, this->ItemDescription, tft.width() / 2, textPosY);
}


void SettingsPage::drawPage() {
	startingItem = 0;  // reset scrolling for initial draw

	tft.fillScreen(BLACK);

	tft.setTextColor(BLUE, BLACK);
	tft.setTextSize(2);
	tft.setCursor(20, 20);
	tft.println("SETTINGS");

	drawItems();
	// drawScrollIndicator();  // skipping this because it's drawn over by the button icons anyways
}

void SettingsPage::redraw() {
	// drawing over *only* the items, rather than the entire page (title, cursor, button prompts)
	tft.fillRect(15, SettingsOption::getYPosition(0) - 5, 240, SettingsOption::getYPosition(ItemsPerPage), BLACK);
	
	drawItems();
	drawScrollIndicator();
}

void SettingsPage::drawItems() {
	SettingsOption* ptr = SettingsOption::getHead();
	size_t position = 0;

	while (ptr != nullptr) {
		if (position >= startingItem) {
			ptr->drawItem(position - startingItem);
		}
		position++;
		ptr = ptr->getNext();

		if (position > startingItem + SettingsPage::ItemsPerPage - 1) break;  // break when we've filled the page (-1 for zero index)
	}
}

void SettingsPage::drawCursor(unsigned int pos) {
	SettingsOption* ptr = SettingsOption::getIndex(pos);
	if (ptr == nullptr) return;  // out of range

	// Find if we're offscreen and adjust items accordingly
	if (pos < startingItem) {
		startingItem--;
		redraw();
	}
		startingItem++;
	else if (pos > startingItem + ItemsPerPage - 1) {  // -1 for zero index
		redraw();
	}

	const unsigned int selectedPos = pos - startingItem;

	// Draw the actual cursor
	tft.setTextColor(BLUE, BLACK);
	tft.setTextSize(2);
	tft.fillRect(0, 20, 20, tft.height() - 20, BLACK);
	tft.setCursor(5, SettingsOption::getYPosition(selectedPos));
	tft.println(">");

	ptr->drawDescription();
}

void SettingsPage::changeOption(unsigned int pos) {
	SettingsOption* ptr = SettingsOption::getIndex(pos);
	if (ptr == nullptr) return;

	ptr->setFunction();

	if (ptr->RefreshOnChange == true) {
		ptr->drawItem(SettingsOption::getPositionOf(ptr) - startingItem);
	}
}

void SettingsPage::drawScrollIndicator() {
	if (SettingsOption::getCount() <= ItemsPerPage) return;  // no scrolling = no scroll indicator

	const unsigned int Width = 15;
	const unsigned int Height = 3;
	const unsigned int MarginY = 2;
	
	const unsigned int XPos = tft.width() - (Width*2);
	const unsigned int YMin = buttonPosY[2] + buttonHeight + MarginY;  // inside edge
	const unsigned int YMax = buttonPosY[3] - MarginY;  // outside edge
	//const unsigned int MaxYSpace = YMax - YMin;

	const unsigned int NumPositions = (SettingsOption::getCount() - 1) - ItemsPerPage;  // -1 for zero index

	const unsigned int yPos = map(startingItem, 0, NumPositions, YMin, YMax);

	tft.fillRect(XPos, YMin, Width, YMax - YMin + Height, BLACK);  // clear area
	tft.fillRect(XPos + Width / 2, YMin, 1, YMax - YMin + Height, WHITE);  // draw center line
	tft.fillRect(XPos, yPos, Width, Height, BLUE);  // draw indicator
}
