#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "track.h"

#define flagSet(A) (isFlagSet(argc, argv, A))
#define getArgOrDefault(A, B) ((getArgument(argc, argv, A) == NULL) ? B : getArgument(argc, argv, A))
#define NOW (time(NULL))

char* getArgument(int argc, char** argv, char* flag){
	int i;
	for(i = 0; i < argc - 1; i ++){
		if(strcmp(argv[i], flag) == 0) return argv[i + 1];
	}
	return NULL;
}

int isFlagSet(int argc, char** argv, char* flag){
	int i;
	for(i = 0; i < argc; i ++){
		if(strcmp(argv[i], flag) == 0) return 1;
	}
	return 0;
}

void writeEntry(FILE* file, Entry* entry){
	fprintf(file, "%d,%d,%s\n", entry->start, entry->end, entry->taskName);
}

Entry* newEntry(){
	Entry* entry = malloc(sizeof(Entry));
	entry->taskName = NULL;
	entry->start = 0;
	entry->end = 0;
}

Entry* readEntry(char* line){
	Entry* entry = newEntry();

	// To avoid buffer overflows, we make the string as long as the string passed in is.
	// This wastes some space, but since the last two items are integers, the number of wasted bytes is no more than 4+2*(max length of a printed integer)
	entry->taskName = malloc(sizeof(char) * (1 + strlen(line)));

	sscanf(line, "%d,%d,%[^\n]s\n", &entry->start, &entry->end, entry->taskName);

	return entry;
}

void addEntry(Entries* entries, Entry* entry){
	if(entries->num == 0){
		entries->entries = malloc(sizeof(Entry) * 1);
	}else{
		entries->entries = realloc(entries->entries, sizeof(Entry) * (entries->num + 1));
	}
	entries->entries[entries->num ++] = entry;
}

void writeEntries(char* fileName, Entries* entries){
	FILE* file = fopen(fileName, "w");

	int i;
	for(i = 0; i < entries->num; i ++){
		writeEntry(file, entries->entries[i]);
	}

	fclose(file);
}

Entries* loadEntries(char* fileName){
	Entries* entries = malloc(sizeof(Entries));
	entries->num = 0;

	Entry* entry;
	FILE* file = fopen(fileName, "r");

	if(file == NULL) return entries;

	char* line;
	size_t bytes;
	while(getline(&line, &bytes, file) > 0){
		entry = readEntry(line);
		addEntry(entries, entry);
	}

	fclose(file);
	return entries;
}

void printDifferenceBetweenTimestamps(unsigned int a, unsigned int b){
	int difference = (b - a);
	int numHours = 0;
	int numMinutes = 0;
	int numSeconds;

	numHours = difference / (60 * 60);
	difference -= (numHours * (60 * 60));
	numMinutes = difference / 60;
	difference -= (numMinutes * 60);
	numSeconds = difference;

	printf("%d hours, %d minutes, and %d seconds", numHours, numMinutes, numSeconds);
}

void cmd_start(char* fileName, Entries* entries, char* taskName){
	Entry* entry = newEntry();
	entry->taskName = taskName;
	entry->start = NOW;

	addEntry(entries, entry);
	writeEntries(fileName, entries);

	printf("Beginning a task \"%s\".\n", taskName);
}

Entry* findFirstOngoingEntry(Entries* entries){
	int i;
	// We iterate through all of the entries from beginning to end, and find the first entry which has an
	// end time of 0.
	// This entry is what we determine the unstopped end time to be.
	for(i = 0; i < entries->num; i ++){
		if(entries->entries[i]->end == 0) return entries->entries[i];
	}
	return NULL;
}

void cmd_end(char* fileName, Entries* entries){
	Entry* firstOngoing = findFirstOngoingEntry(entries);
	if(firstOngoing == NULL){
		printf("No ongoing tasks.\n");
		return; // There are none, so we stop.
	}

	firstOngoing->end = NOW;

	writeEntries(fileName, entries);

	printf("Ended task \"%s\": ", firstOngoing->taskName);
	printDifferenceBetweenTimestamps(firstOngoing->start, firstOngoing->end);
	printf(".\n");
}

void cmd_print(Entries* entries, char* numDays){
	int numDaysInt = atoi(numDays);
	unsigned int earliestTime = NOW - (numDaysInt * 60 * 60 * 24);
	int i, totalDuration = 0;
	Entry* entry;

	for(i = 0; i < entries->num; i ++){
		entry = entries->entries[i];
		if(entry->start >= earliestTime){
			printf("Task \"%s\": ", entry->taskName);
			if(entry->end == 0){
				printDifferenceBetweenTimestamps(entry->start, NOW);
				totalDuration += (NOW - entry->start);
			}else{
				printDifferenceBetweenTimestamps(entry->start, entry->end);
				totalDuration += (entry->end - entry->start);
			}
			printf(".\n");
		}
	}

	printf("Total time: ");
	printDifferenceBetweenTimestamps(0, totalDuration);
	printf(".\n");
}

void cmd_duration(Entries* entries){
	Entry* firstOngoing = findFirstOngoingEntry(entries);
	if(firstOngoing == NULL){
		printf("No ongoing tasks.\n");
		return; // There are none, so we stop.
	}

	printf("Task \"%s\" has been ongoing for: ", firstOngoing->taskName);
	printDifferenceBetweenTimestamps(firstOngoing->start, NOW);
	printf(".\n");
}

void cmd_resume(char* fileName, Entries* entries){
	if(entries->num == 0){
		printf("There are no entries to resume.\n");
		return;
	}
	cmd_start(fileName, entries, entries->entries[entries->num - 1]->taskName);

}

int main(int argc, char** argv){

	int startFlag = flagSet("-s");
	int endFlag = flagSet("-e");
	int durationFlag = flagSet("-d");
	int printFlag = flagSet("-p");
	int resumeFlag = flagSet("-r");
	char* fileName = getArgOrDefault("-f", "time.txt");
	char* taskName = getArgOrDefault("-t", "null");
	char* printDuration = getArgOrDefault("-n", "14");

	if(argc == 1 || flagSet("-h") == 1 || flagSet("-help") == 1){
		printf("Usage: %s <flags>\n", argv[0]);
		printf("Flags: \n");
		printf("-s: Starts a timer. Use -t to pass a task name.\n");
		printf("-e: Ends the latest timer.\n");
		printf("-d: Duration. If a task is currently ongoing, detects how long it's been occuring for.\n");
		printf("-p: Print. Prints all of the events within the last -n days.\n");
		printf("-r: Resume. Copies the last entry name that occured, and resumes it at the current time.\n");
		printf("-f: The file to use for storage. Defaults to \"time.txt\"\n");
		printf("-t: The task name to utilize. Defaults to \"null\"\n");
		printf("-n: The number of days to print. Defaults to 14.\n");
		return 0;
	}

	Entries* entries = loadEntries(fileName);

	if(startFlag){
		cmd_start(fileName, entries, taskName);
	}else if(endFlag){
		cmd_end(fileName, entries);
	}else if(printFlag){
		cmd_print(entries, printDuration);
	}else if(durationFlag){
		cmd_duration(entries);
	}else if(resumeFlag){
		cmd_resume(fileName, entries);
	}

}
