#ifndef ENTRY_H_
	#define ENTRY_H_

	struct Entry{
	        char* taskName;
	        time_t start;
	        time_t end;
	};

	typedef struct Entry Entry;

	struct Entries{
	        int num;
	        Entry** entries;
	};

	typedef struct Entries Entries;


	void writeEntry(FILE*, Entry*);
	Entry* newEntry();
	Entry* readEntry(char*);
	void addEntry(Entries*, Entry*);
	void writeEntries(char*, Entries*);
	Entries* loadEntries(char* fileName);
	Entry* findFirstOngoingEntry(Entries*);
	Entry* duplicateEntry(Entry*);
	Entries** getEntriesPerWeekday(int daysBack, Entries* entries);
	time_t* getDayTimestamps(int daysBack);

#endif
