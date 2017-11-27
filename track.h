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

