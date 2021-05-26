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
	const unsigned int xPos = ItemStartX;
	const unsigned int yPos = getYPosition(position);

	int16_t clearX, clearY;
	uint16_t clearWidth, clearHeight;

	tft.setTextSize(2);
	tft.getTextBounds(ItemName + " ", xPos, yPos, &clearX, &clearY, &clearWidth, &clearHeight);
	tft.fillRect(clearX, clearY, clearWidth, clearHeight, BLACK);  // clear text area

	tft.setCursor(xPos, yPos);

	tft.setTextColor(WHITE, BLACK);
	tft.print(ItemName);
	tft.print(' ');

	this->drawValue(position);
}

void SettingsOption::drawValue(unsigned int position) {
	const unsigned int yPos = getYPosition(position);

	int16_t nameX, dummyY;
	uint16_t nameWidth, dummyHeight;

	// Calculate size of item name to set cursor at end
	tft.setTextSize(2);
	tft.getTextBounds(ItemName + " ", ItemStartX, yPos, &nameX, &dummyY, &nameWidth, &dummyHeight);

	const unsigned int xPos = nameX + nameWidth;  // start at the end of the item name

	// Note that we're not reusing the variables from above because the program seems to get
	// confused and use the values from the first call rather than the second (potentially an optimization issue?)
	int16_t clearX, clearY;
	uint16_t clearWidth, clearHeight;

	// Calculate size of item description for clearing (using last value)
	tft.setTextSize(2);
	tft.getTextBounds(lastValue, xPos, yPos, &clearX, &clearY, &clearWidth, &clearHeight);
	tft.fillRect(clearX, clearY, clearWidth, clearHeight, BLACK);  // clear area

	tft.setCursor(xPos, yPos);
	tft.setTextColor(YELLOW, BLACK);
	tft.print(getFunction());

	lastValue = getFunction();  // save value so we know how much to 'clear' for next call
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


void SettingsPage::drawPage(unsigned int pos) {
	startingItem = 0;  // reset pagination
	updateScroll(pos);  // recalculate pagination for current position

	tft.fillScreen(BLACK);

	tft.setTextColor(BLUE, BLACK);
	tft.setTextSize(2);
	tft.setCursor(20, 20);
	tft.println("SETTINGS");

	drawItems();
	// drawScrollIndicator();  // skipping this because it's drawn over by the button icons anyways
}

void SettingsPage::redraw(unsigned int pos) {
	// drawing over *only* the items, rather than the entire page (title, cursor, button prompts)
	tft.fillRect(15, SettingsOption::getYPosition(0) - 5, 240, SettingsOption::getYPosition(ItemsPerPage), BLACK);

	// clear the description box as well
	tft.fillRect(0, tft.height() - 20, tft.width(), 20, BLACK);

	// recalculate pagination for current position (if necessary)
	updateScroll(pos);
	
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
	SettingsOption* ptr = SettingsOption::getItemAtIndex(pos);
	if (ptr == nullptr) return;  // out of range

	// Clear cursor
	tft.fillRect(0, 20, 20, tft.height() - 20, BLACK);

	// If the current selection is not on the page, we need to redraw the list accordingly
	if (updateScroll(pos)) {
		redraw(pos);  // redraws those items on the screen
	}

	const unsigned int selectedPos = pos - startingItem;

	// Draw the actual cursor
	tft.setTextColor(BLUE, BLACK);
	tft.setTextSize(2);
	tft.setCursor(5, SettingsOption::getYPosition(selectedPos));
	tft.println(">");

	ptr->drawDescription();
}

void SettingsPage::changeOption(unsigned int pos) {
	SettingsOption* ptr = SettingsOption::getItemAtIndex(pos);
	if (ptr == nullptr) return;

	ptr->setFunction();

	if (ptr->RefreshOnChange == true) {
		ptr->drawValue(SettingsOption::getPositionOf(ptr) - startingItem);
	}
}

bool SettingsPage::updateScroll(unsigned int pos) {
	if (onPage(pos)) return false;  // already on page, don't update

	switch (Scroll) {
	case(ScrollType::Smooth):
		if (pos < startingItem) {
			// 'decrement' to current as first
			startingItem = pos;
		}
		else if (pos > lastItem()) {
			// 'increment' to position as last
			startingItem = pos - ItemsPerPage + 1;  // +1 for 0 index
		}
		break;
	case(ScrollType::Paged):
	{
		const unsigned int Page = (pos / ItemsPerPage);  // page for this item
		const unsigned int PageStart = Page * ItemsPerPage;  // starting item for that page
		startingItem = PageStart;
		break;
	}
	}

	return true;
}

void SettingsPage::drawScrollIndicator() {
	if (SettingsOption::getCount() <= ItemsPerPage) return;  // no scrolling = no scroll indicator

	switch (Scroll) {
	case(ScrollType::Smooth):
	{
		const unsigned int NumPositions = SettingsOption::getCount() - ItemsPerPage;
		const float value = (float) startingItem / (float) NumPositions;
		
		ShowScrollIndicator(value, BLUE);
		break;
	}
	case(ScrollType::Paged):
	{
		const unsigned int Page = (startingItem / ItemsPerPage) + 1;
		const unsigned int NumItems = SettingsOption::getCount();
		const unsigned int NumPages = (NumItems / ItemsPerPage) + ((NumItems % ItemsPerPage != 0) ? 1 : 0);
		ShowPageIndicator(Page, NumPages, WHITE);
		break;
	}
	}
}
