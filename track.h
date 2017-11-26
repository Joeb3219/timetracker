struct Entry{
	char* taskName;
	int start;
	int end;
};

typedef struct Entry Entry;

struct Entries{
	int num;
	Entry** entries;
};

typedef struct Entries Entries;

