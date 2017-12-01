#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "entry.h"

#define NOW (time(NULL))

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

Entry* duplicateEntry(Entry* entry){
        Entry* dup = malloc(sizeof(Entry));
        dup->start = entry->start;
        dup->end = entry->end;
        dup->taskName = strdup(entry->taskName);
        return dup;
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

