template<typename T>
class LinkedList {
private:
	static T* head;
	T* next = nullptr;

public:
	static T* getHead() { return head; }
	T* getNext() { return next; }

	// indexed at 1
	static size_t getCount() {
		size_t count = 0;
		T* ptr = head;
		while (ptr != nullptr) {
			ptr = ptr->next;
			count++;
		}
		return count;
	}

	static T* getIndex(unsigned int i) { 
		T* ptr = head;
		while (ptr != nullptr && i != 0) {
			ptr = ptr->getNext();
			i--;
		}
		if (i != 0) return nullptr;
		return ptr;
	}

	static int getPositionOf(T* node) {
		unsigned int position = 0;
		T* ptr = head;
		while (ptr != nullptr) {
			if (node == ptr) return position;
			position++;
			ptr = ptr->getNext();
		}
		return -1;  // not found
	}

protected:
	static void add(T* node) {
		if (head == nullptr) {
			head = node;
		}
		else {
			T* last = head;
			while (true) {
				if (last->next == nullptr) break;  // found last entry
				last = last->next;
			}
			last->next = node;
		}
	}

	static void remove(T* node) {
		if (node == head) {
			head = node->next; // Set head to next, and we're done
			return;
		}

		// Otherwise we're somewhere else in the list. Iterate through to find it.
		T* ptr = head;

		while (ptr != nullptr) {
			if (ptr->next == node) {  // FOUND!
				ptr->next = node->next;  // Set the previous "next" as this entry's "next" (skip this object)
				break;  // Stop searching
			}
			ptr = ptr->next;  // Not found. Next entry...
		}
	}
};
