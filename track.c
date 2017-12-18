#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "entry.h"

#define flagSet(A) (isFlagSet(argc, argv, A))
#define getArgOrDefault(A, B) ((getArgument(argc, argv, A) == NULL) ? B : getArgument(argc, argv, A))
#define NOW (time(NULL))

#define CLR_RED   "\x1B[31m"
#define CLR_GREEN   "\x1B[32m"
#define CLR_ORANGE   "\x1B[33m"
#define CLR_RESET "\x1B[0m"

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

	printf(CLR_ORANGE "%d" CLR_RESET " hours, " CLR_ORANGE "%d" CLR_RESET " minutes, and " CLR_ORANGE "%d" CLR_RESET " seconds", numHours, numMinutes, 
numSeconds);
}

void cmd_start(char* fileName, Entries* entries, char* taskName){
	Entry* entry = newEntry();
	entry->taskName = taskName;
	entry->start = NOW;

	addEntry(entries, entry);
	writeEntries(fileName, entries);

	printf("Beginning a task " CLR_RED "%s" CLR_RESET ".\n", taskName);
}

void cmd_end(char* fileName, Entries* entries){
	Entry* firstOngoing = findFirstOngoingEntry(entries);
	if(firstOngoing == NULL){
		printf("No ongoing tasks.\n");
		return; // There are none, so we stop.
	}

	firstOngoing->end = NOW;

	writeEntries(fileName, entries);

	printf("Ended task " CLR_RED "%s" CLR_RESET ": ", firstOngoing->taskName);
	printDifferenceBetweenTimestamps(firstOngoing->start, firstOngoing->end);
	printf(".\n");
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
			printf("== %d/%d/%d ==\n", (1900 + ts->tm_year), (ts->tm_mon + 1), ts->tm_mday);
		}

		for(j = 0; j < dayEntries->num; j ++){

			ts = localtime(&dayEntries->entries[j]->start);

			printf(CLR_RED "%s" CLR_RESET, dayEntries->entries[j]->taskName);
			printf(" (" CLR_ORANGE "%02d" CLR_RESET ":" CLR_ORANGE "%02d" CLR_RESET "): \t", ts->tm_hour, ts->tm_min);
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
			printf(CLR_GREEN "(%f hours)" CLR_RESET, (dailyDuration / (60.0 * 60.0)));
			printf("\n\n");
		}

		// Free the entries
		if(dayEntries->num > 0) free(dayEntries->entries);
		free(dayEntries);
	}

	printf("Total time: ");
	printDifferenceBetweenTimestamps(0, totalDuration);
	printf(CLR_GREEN " (%f hours).\n" CLR_RESET, (totalDuration / (60.0 * 60.0)));

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
