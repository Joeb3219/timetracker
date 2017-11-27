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

	char* line = NULL;
	size_t bytes;
	while(getline(&line, &bytes, file) > 0){
		entry = readEntry(line);
		addEntry(entries, entry);
	}
	free(line);

	fclose(file);
	return entries;
}

void printDifferenceBetweenTimestamps(time_t a, time_t b){
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

time_t* getDayTimestamps(int daysBack){
	int totalDays = daysBack + 1; // We want to include today, so we need an extra day.
	int i = 0;
	time_t* timestamps = malloc(sizeof(time_t) * totalDays);

	time_t currenttimestamp;
	struct tm* ts;
	for(i = 0; i < totalDays; i ++){
		currenttimestamp = NOW;
		ts = localtime(&currenttimestamp);
		ts->tm_mday -= i;
		ts->tm_hour = ts->tm_min = ts->tm_sec = 0;
		currenttimestamp = mktime(ts);
		timestamps[totalDays - i - 1] = currenttimestamp;
	}

	return timestamps;
}

Entry* duplicateEntry(Entry* entry){
	Entry* dup = malloc(sizeof(Entry));
	dup->start = entry->start;
	dup->end = entry->end;
	dup->taskName = strdup(entry->taskName);
	return dup;
}

Entries** getEntriesPerWeekday(int daysBack, Entries* entries){
	int totalDays = daysBack + 1; // We want to include today, so we need an extra day.
	Entries** days = malloc(sizeof(Entries*) * totalDays);
	time_t* timestamps = getDayTimestamps(daysBack);
	int i = 0, j = 0;
	time_t endstamp;
	Entry* entry;

	for(i = 0; i < totalDays; i ++){
		days[i] = malloc(sizeof(Entries));
		days[i]->num = 0;
		endstamp = timestamps[i] + (24 * 60 * 60);

		for(j = 0; j < entries->num; j ++){
			// First case is when the event is entirely within the group, in which case we just
			if(entries->entries[j]->end == 0){
				if(entries->entries[j]->start >= timestamps[i] && entries->entries[j]->start < endstamp){
					entry = duplicateEntry(entries->entries[j]);
					entry->end = NOW;
					addEntry(days[i], entry);
				}
			}else if(entries->entries[j]->start >= timestamps[i] && entries->entries[j]->end <= endstamp){
				addEntry(days[i], duplicateEntry(entries->entries[j]));

			}else if(entries->entries[j]->start >= timestamps[i] && entries->entries[j]->start < endstamp){
				entry = duplicateEntry(entries->entries[j]);
				entry->end = endstamp;
				addEntry(days[i], entry);

			}else if(entries->entries[j]->end <= endstamp && entries->entries[j]->end > timestamps[i]){
				entry = duplicateEntry(entries->entries[j]);
				entry->start = timestamps[i];
				addEntry(days[i], entry);

			}

		}

	}

	free(timestamps);
	return days;
}

void cmd_print(Entries* entries, char* numDays){
	int numDaysInt = atoi(numDays);
	time_t earliestTime = NOW - (numDaysInt * 60 * 60 * 24);
	int i, j, totalDuration = 0, dailyDuration = 0;
	Entry* entry;

	Entries** dailyEntries = getEntriesPerWeekday(numDaysInt, entries);
	Entries* dayEntries;
	time_t* timestamps = getDayTimestamps(numDaysInt);
	struct tm* ts;


	for(i = 0; i <= numDaysInt; i ++){
		dailyDuration = 0;
		dayEntries = dailyEntries[i];
		if(dayEntries->num > 0){
			ts = localtime(&timestamps[i]);
			printf("== %d/%d/%d ==\n", (1900 + ts->tm_year), ts->tm_mon, ts->tm_mday);
		}

		for(j = 0; j < dayEntries->num; j ++){

			ts = localtime(&dayEntries->entries[j]->start);

			printf("Task \"%s\" (%d:%d:%d): \t", dayEntries->entries[j]->taskName, ts->tm_hour, ts->tm_min, ts->tm_sec);
			if(dayEntries->entries[j]->end != 0){
				printDifferenceBetweenTimestamps(dayEntries->entries[j]->start, dayEntries->entries[j]->end);
				dailyDuration += (dayEntries->entries[j]->end - dayEntries->entries[j]->start);
			}
			else{
				printDifferenceBetweenTimestamps(dayEntries->entries[j]->start, NOW);
				dailyDuration += (NOW - dayEntries->entries[j]->start);
			}
			printf(".\n");

			// Free the entry
			free(dayEntries->entries[j]->taskName);
			free(dayEntries->entries[j]);
		}

		totalDuration += dailyDuration;
		if(dayEntries->num > 0){
			printf("Daily time: ");
			printDifferenceBetweenTimestamps(0, dailyDuration);
			printf(" (%f hours).\n", (dailyDuration / (60.0 * 60.0)));
			printf("\n");
		}

		// Free the entries
		if(dayEntries->num > 0) free(dayEntries->entries);
		free(dayEntries);
	}

	printf("Total time: ");
	printDifferenceBetweenTimestamps(0, totalDuration);
	printf(" (%f hours).\n", (totalDuration / (60.0 * 60.0)));

	// Free the entries array
	free(dailyEntries);
	free(timestamps);
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
	cmd_start(fileName, entries, strdup(entries->entries[entries->num - 1]->taskName));

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
		cmd_start(fileName, entries, strdup(taskName));
	}else if(endFlag){
		cmd_end(fileName, entries);
	}else if(printFlag){
		cmd_print(entries, printDuration);
	}else if(durationFlag){
		cmd_duration(entries);
	}else if(resumeFlag){
		cmd_resume(fileName, entries);
	}


	// Now free up all of the memory we allocated.
	int i;
	for(i = 0; i < entries->num; i ++){
		free(entries->entries[i]->taskName);
		free(entries->entries[i]);
	}
	free(entries->entries);
	free(entries);

}
