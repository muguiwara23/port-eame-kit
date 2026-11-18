/* Port/eame kit: the iOS app's shared services (porteame/ios.h). From the
 * Yu-Gi-Oh! Forbidden Memories (ios.c) and EarthBound
 * (desktop_application.cpp, desktop_display.cpp) ports.
 *
 * PE_TEST_IOS builds the iOS paths on another system, for the kit's tests. */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE /* setenv, strcasecmp off Apple's headers */
#endif
#include "porteame/ios.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#if defined(SDL_PLATFORM_IOS) || defined(PE_TEST_IOS)
#define PE_IOS 1
#else
#define PE_IOS 0
#endif

#if PE_IOS
static char documents[1024];

static void read_environment(void)
{
    char path[1100], line[700];
    FILE *file;
    snprintf(path, sizeof(path), "%senvironment.txt", documents);
    if (!(file = fopen(path, "r"))) return;
    while (fgets(line, sizeof(line), file)) {
        char *equals;
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '#' || !(equals = strchr(line, '=')) || equals == line) continue;
        *equals = '\0';
        setenv(line, equals + 1, 1);
        fprintf(stderr, "Port/eame: environment.txt sets %s\n", line);
    }
    fclose(file);
}
#endif

const char *pe_ios_startup(const char *app_name, const char *const *folders)
{
#if PE_IOS
    const char *home = getenv("HOME");
    char path[1100], previous[1100];
    time_t now = time(NULL);
    int log;
    if (!home || !*home) return NULL;
    snprintf(documents, sizeof(documents), "%s%sDocuments/", home, home[strlen(home) - 1] == '/' ? "" : "/");
    mkdir(documents, 0755);
    for (; folders && *folders; folders++) {
        snprintf(path, sizeof(path), "%s%s", documents, *folders);
        mkdir(path, 0755);
    }
    /* The last run's log is kept: the one worth reading after a crash. */
    snprintf(path, sizeof(path), "%sport-eame-log.txt", documents);
    snprintf(previous, sizeof(previous), "%sport-eame-log-previous.txt", documents);
    rename(path, previous);
    log = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (log >= 0) {
        fflush(stdout);
        fflush(stderr);
        dup2(log, STDOUT_FILENO);
        dup2(log, STDERR_FILENO);
        close(log);
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
    }
    fprintf(stderr, "Port/eame %s: started %s", app_name ? app_name : "", ctime(&now));
    fprintf(stderr, "Port/eame: documents %s\n", documents);
    if (getcwd(path, sizeof(path))) fprintf(stderr, "Port/eame: bundle %s\n", path);
    read_environment();
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    return documents;
#else
    (void)app_name;
    (void)folders;
    return NULL;
#endif
}

const char *pe_ios_documents(void)
{
#if PE_IOS
    return documents[0] ? documents : NULL;
#else
    return NULL;
#endif
}

static int has_extension(const char *name, const char *const *extensions)
{
    size_t length = strlen(name);
    for (; *extensions; extensions++) {
        size_t e = strlen(*extensions);
        if (length > e && !strcasecmp(name + length - e, *extensions)) return 1;
    }
    return 0;
}

static int is_directory(const char *path)
{
    struct stat info;
    return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

static int is_file(const char *path)
{
    struct stat info;
    return stat(path, &info) == 0 && S_ISREG(info.st_mode);
}

/* The first match by name in one folder (depth 0) or one level below. */
static int find_in(const char *folder, const char *const *extensions, int depth, char *best, size_t size)
{
    DIR *dir = opendir(folder);
    struct dirent *entry;
    int found = 0;
    char path[2048];
    if (!dir) return 0;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;
        snprintf(path, sizeof(path), "%s%s%s", folder, folder[strlen(folder) - 1] == '/' ? "" : "/", entry->d_name);
        if (has_extension(entry->d_name, extensions) && is_file(path)) {
            if (!best[0] || strcmp(path, best) < 0) snprintf(best, size, "%s", path);
            found = 1;
        } else if (depth == 0 && is_directory(path)) {
            found |= find_in(path, extensions, 1, best, size);
        }
    }
    closedir(dir);
    return found;
}

int pe_ios_find_file(const char *folder, const char *const *extensions, char *out, size_t size)
{
    if (!folder || !*folder || !extensions || !out || !size) return 0;
    out[0] = '\0';
    return find_in(folder, extensions, 0, out, size) && out[0];
}

SDL_WindowFlags pe_ios_window_flags(void)
{
#if PE_IOS
    return SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
#else
    return 0;
#endif
}

float pe_ios_density(SDL_Window *window)
{
    float density = window ? SDL_GetWindowPixelDensity(window) : 0.0f;
    return density > 0.0f ? density : 1.0f;
}

void pe_ios_safe_area(SDL_Window *window, int *left, int *right, int *bottom)
{
    SDL_Rect safe;
    int w = 0, h = 0;
    float density = pe_ios_density(window);
    *left = *right = *bottom = 0;
    if (!window || !SDL_GetWindowSafeArea(window, &safe) || !SDL_GetWindowSize(window, &w, &h) || w <= 0 || h <= 0) return;
    *left = (int)((float)safe.x * density);
    *right = (int)((float)(w - safe.x - safe.w) * density);
    *bottom = (int)((float)(h - safe.y - safe.h) * density);
    if (*left < 0) *left = 0;
    if (*right < 0) *right = 0;
    if (*bottom < 0) *bottom = 0;
}

int pe_ios_is_background(const SDL_Event *event)
{
    return event->type == SDL_EVENT_WILL_ENTER_BACKGROUND || event->type == SDL_EVENT_DID_ENTER_BACKGROUND;
}

int pe_ios_wait_for_foreground(void (*save)(void *userdata), void *userdata)
{
    SDL_Event event;
    fprintf(stderr, "Port/eame: in the background, paused\n");
    if (save) save(userdata);
    while (SDL_WaitEvent(&event)) {
        if (event.type == SDL_EVENT_DID_ENTER_FOREGROUND) {
            fprintf(stderr, "Port/eame: in the foreground, resumed\n");
            return 1;
        }
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_TERMINATING) return 0;
    }
    return 0;
}

void pe_ios_error(const char *title, const char *message)
{
    fprintf(stderr, "%s: %s\n", title ? title : "Error", message ? message : "");
#if PE_IOS
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title ? title : "Error", message ? message : "", NULL);
#endif
}

int pe_ios_finish(int status)
{
#if PE_IOS
    fprintf(stderr, "Port/eame: closed (%d)\n", status);
    fflush(stdout);
    fflush(stderr);
    exit(status);
#else
    return status;
#endif
}
