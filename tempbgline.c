/*
 * tempbgline prints a one line unicode bargraph displaying the temperature of each CPU core
 * Copyright (C) 2017  Ralf Stemmer <ralf.stemmer@gmx.net>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Includes
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#define VERSION "1.0.0"

const char *TILES[9] = {" ","▁","▂","▃","▄","▅","▆","▇","█"};

// for intel CPUs only - very hacky
// Related docu:
//  * https://www.kernel.org/doc/Documentation/hwmon/coretemp
//  * https://www.kernel.org/doc/Documentation/hwmon/sysfs-interface

struct TEMPLIST
{
    unsigned int temp;
    unsigned int high;
    unsigned int crit;
    char *label;
    int corenr;    // if label is "Core X", this variable contains X, otherwise -1

    struct TEMPLIST *next;
};



void* SafeMAlloc(size_t size);
bool isValidDirectory(const char *path);
char* GetHardwareMonitorPath(void);
int ReadIntegerFromFile(const char *path);
char* ReadStringFromFile(const char *path);
int GetTemperature(const char *hwmonpath, struct TEMPLIST **templist);
int GetAverageTemperature(const char *hwmonpath, struct TEMPLIST **templist);
void ShowTemperature(struct TEMPLIST *templist, int numofcores);
void ShowAverageTemperature(struct TEMPLIST *templist, int numofcores);



int main(int argc, char *argv[])
{
    char *hwmonpath  = NULL;
    bool opt_average = false;
    bool opt_verbose = false;

    // Read Parameter
    for(int argi = 1; argi < argc; argi++)
    {
        if(argv[argi][0] != '-')
        {
            fprintf(stderr, "\e[1;31mUnexpected argument \"%s\"!\e[0m\n", argv[argi]);
            exit(EXIT_FAILURE);
        }

        switch(argv[argi][1])
        {
            case 'm':
                hwmonpath = argv[++argi];
                break;

            case 'a':
                opt_average = true;
                break;

            case 'v':
                opt_verbose = true;
                break;

            default:
                fprintf(stderr, "\e[1;31mUnexpected argument \"%s\"!\e[0m\n", argv[argi]);
                exit(EXIT_FAILURE);
        }
    }

    // Get Hardware Monitor Path
    if(hwmonpath == NULL)
        hwmonpath = GetHardwareMonitorPath();
 
    if(opt_verbose)
        printf("\e[1;34mHardware Monitor: \e[0;36m%s\e[0m\n", hwmonpath);
    
    // Get Temperature
    int corecnt;
    struct TEMPLIST *templist;
    corecnt = GetTemperature(hwmonpath, &templist);
    if(corecnt < 1)
        exit(EXIT_FAILURE);

    if(opt_verbose)
        printf("\e[1;34mNumber of Cores:  \e[0;36m%i\e[0m\n", corecnt);

    if(opt_verbose)
    {
        struct TEMPLIST *tl;
        tl = templist;
        printf("\e[1;34mLabel\t\t Core\t Temp\t High\t Crit\n\e[0;36m");
        while(tl != NULL)
        {
            printf(" %s \t%7i\t%7i\t%7i\t%7i\n", tl->label, tl->corenr, tl->temp, tl->high, tl->crit);
            tl = tl->next;
        }
        printf("\e[1;31mGraph [\e[1;34m");
    }

    // Show Bar Graph
    if(opt_average)
        ShowAverageTemperature(templist, corecnt);
    else
        ShowTemperature(templist, corecnt);

    if(opt_verbose)
        printf("\e[1;31m]\e[0m\n");
    return EXIT_SUCCESS;
}



bool isValidDirectory(const char *path)
{
    struct stat st;
    int retval;

    retval = stat(path, &st);
    if(retval != 0)
        return false;

    if((st.st_mode & S_IFDIR) == 0)
        return false;

    return true;
}



// Assuming there is exact one coretemp directory and exact one hwmon entry
char* GetHardwareMonitorPath(void)
{
    const int LIMIT = 10;
    // Find the correct "coretemp.X" directory - on all my intel systems it is "coretemp.0" - non-intel-systems don't have coretemp.X at all
    char *coretemppath;
    for(int coretempnr=0; coretempnr < LIMIT; coretempnr++)
    {
        asprintf(&coretemppath, "/sys/devices/platform/coretemp.%i", coretempnr);
        if(isValidDirectory(coretemppath))
            break;

        free(coretemppath);
        coretemppath = NULL;
    }

    if(coretemppath == NULL)
    {
        fprintf(stderr, "\e[1;31mCannot find a valid /sys/devices/platform/coretem.* directory. Don\'t you use an Intel CPU?\e[0m\n");
        return NULL;
    }

    // Find the correct hardware monitor
    char *hwmonpath;
    for(int hwmonnr=0; hwmonnr < LIMIT; hwmonnr++) // for each hwmonX - there will be just one entry, but its number is unpredictable
    {
        asprintf(&hwmonpath, "%s/hwmon/hwmon%i", coretemppath, hwmonnr);
        if(isValidDirectory(hwmonpath))
            break;

        free(hwmonpath);
        hwmonpath = NULL;
    }

    if(hwmonpath == NULL)
    {
        fprintf(stderr, "\e[1;31mCannot find a valid %s/hwmon/hwmon* directory.\e[0m\n", coretemppath);
        return NULL;
    }

    free(coretemppath); // not needed anymore
    return hwmonpath;
}



int ReadIntegerFromFile(const char *path)
{
    FILE *fp;
    fp = fopen(path, "r");
    if(fp == NULL)
    {
        fprintf(stderr, "\e[1;31mfopen(%s, \"r\"); failed with error: ", path);
        fprintf(stderr, "\e[1;31m%s\e[0m\n", strerror(errno));
        return -1;
    }

    int integer;
    fscanf(fp, "%i", &integer);
    fclose(fp);
    return integer;
}



char* ReadStringFromFile(const char *path)
{
    FILE *fp;
    fp = fopen(path, "r");
    if(fp == NULL)
    {
        fprintf(stderr, "\e[1;31mfopen(%s, \"r\"); failed with error: ", path);
        fprintf(stderr, "\e[1;31m%s\e[0m\n", strerror(errno));
        return NULL;
    }

    size_t buffersize = 0;
    size_t linelength = 0;
    char  *linebuffer = NULL;
    linelength = getline(&linebuffer, &buffersize, fp);

    fclose(fp);

    if(linelength <= 0)
    {
        fprintf(stderr, "\e[1;31mUnable to read the first line of %s.\e[0m\n", path);
        return NULL;
    }

    // Remove trailing newlines
    while(linebuffer[linelength-1] == '\n')
    {
        linebuffer[linelength-1] = '\0';
        linelength--;
    }
    return linebuffer;
}



// Returns max core count - this will be less than max entries in the list. if negative, an error occured
int GetTemperature(const char *hwmonpath, struct TEMPLIST **templist)
{
    if(hwmonpath == NULL)
        return -1;

    if(templist == NULL)
        return -1;

    *templist = NULL;

    int maxcores = 0;
    for(int i=1; ; i++) // For each temperature sensor (it starts with temp1)
    {
        char *path_temp_crit;
        char *path_temp_max;
        char *path_temp_input;
        char *path_temp_label;

        asprintf(&path_temp_label, "%s/temp%i_label", hwmonpath, i);
        if(access(path_temp_label, F_OK) != 0)  // If it does not exist, all sensors are read.
        {
            free(path_temp_label);

            // On one of my systems, temp1_* is missing.
            if(i == 1)
                continue;
            break;
        }

        asprintf(&path_temp_crit,  "%s/temp%i_crit" , hwmonpath, i);
        asprintf(&path_temp_max,   "%s/temp%i_max"  , hwmonpath, i);
        asprintf(&path_temp_input, "%s/temp%i_input", hwmonpath, i);


        // Create new temp-list entry
        struct TEMPLIST *tlentry;
        tlentry = SafeMAlloc(sizeof(struct TEMPLIST));
        tlentry->temp = ReadIntegerFromFile(path_temp_input);
        tlentry->high = ReadIntegerFromFile(path_temp_max);
        tlentry->crit = ReadIntegerFromFile(path_temp_crit);
        tlentry->label  = ReadStringFromFile(path_temp_label);
        tlentry->corenr = 0;
        if(sscanf(tlentry->label, "Core %i", &tlentry->corenr) != 1)
            tlentry->corenr = -1;

        if(tlentry->corenr + 1 > maxcores)
            maxcores = tlentry->corenr + 1;

        // Add Entry to list
        tlentry->next = *templist;
        *templist = tlentry;

        free(path_temp_crit);
        free(path_temp_max);
        free(path_temp_input);
        free(path_temp_label);
    }

    return maxcores;
}



void ShowTemperature(struct TEMPLIST *templist, int numofcores)
{
    if(templist == NULL || numofcores <= 0)
        return;

    const char **graph;
    graph = SafeMAlloc(sizeof(char*) * numofcores);

    while(templist != NULL)
    {
        if(templist->corenr < 0) // Only print core temperature
        {
            templist = templist->next;
            continue;
        }

        int barnr = templist->corenr;

        // Too hot!
        if(templist->temp > templist->crit)
            graph[barnr] = "\e[1;31m!"; // the escape sequence fucks up the color settings, but this info is important for the user
        else if(templist->temp > templist->high)
            graph[barnr] = "!";
        else
        {
            int tilenr;
            float reltemp = (float)templist->temp / (float)templist->high;
            tilenr = (int)(8 * reltemp + 0.5);
            graph[barnr] = TILES[tilenr];
        }


        templist = templist->next;
    }

    for(int i=0; i<numofcores; i++)
        printf("%s", graph[i]);
}


void ShowAverageTemperature(struct TEMPLIST *templist, int numofcores)
{
    if(templist == NULL || numofcores <= 0)
        return;

    float sum  = 0.0;
    while(templist != NULL)
    {
        if(templist->corenr < 0) // Only print core temperature
        {
            templist = templist->next;
            continue;
        }

        sum += (float)templist->temp / (float)templist->high;
        templist = templist->next;
    }

    int tilenr;
    tilenr = (int)(8 * (sum/numofcores) + 0.5);

    // In case one temp is > high temp, it may exceed the number of tiles. high temp is no upper limit!
    if(tilenr > 8)
        printf("!");
    else
        printf("%s", TILES[tilenr]);
}


void* SafeMAlloc(size_t size)
{
    void *memory = malloc(size);
    if(memory == NULL)
    {
        fprintf(stderr, "\e[1;31mmalloc(%lu); failed with error: ", size);
        fprintf(stderr, "\e[1;31m%s\e[0m\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    memset(memory, 0, size);
    return memory;
}



// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

