#include "project.h"
#include "project_template.h"
#include "../utils/cutils.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

void createProject(const char *name, const char *parent) {
    if (!*parent) exitWithError("Expected a parent directory");
    if (!*name || !strcmp(name, ".") || !strcmp(name, "..") || strpbrk(name, "/\\"))
        exitWithError("Expected a project name, got '%s'", name);
    char *directory = mallocSafe(strlen(parent) + strlen(name) + 2);
    sprintf(directory, "%s/%s", parent, name);
#ifdef _WIN32
    int result = _mkdir(directory);
#else
    int result = mkdir(directory, 0777);
#endif
    if (result) exitWithError("Cannot create project '%s': %s", directory, strerror(errno));
    char *file = mallocSafe(strlen(directory) + sizeof("/main.lt"));
    sprintf(file, "%s/main.lt", directory);
    FILE *output = fopen(file, "wx");
    if (!output) exitWithError("Cannot create '%s': %s", file, strerror(errno));
    size_t written = fwrite(PROJECT_TEMPLATE, 1, sizeof(PROJECT_TEMPLATE), output);
    int closed = fclose(output);
    if (written != sizeof(PROJECT_TEMPLATE) || closed) exitWithError("Cannot write '%s'", file);
    printf("Created project '%s'\n", directory);
    free(file);
    free(directory);
}
