/* Port/eame kit: the iOS helpers, their iOS paths built on Linux
 * (PE_TEST_IOS) and run in a scratch HOME. */
#include "porteame/ios.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static int failures;
#define CHECK(what)                                                            \
    do {                                                                       \
        if (!(what)) {                                                         \
            fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #what); \
            failures++;                                                        \
        }                                                                      \
    } while (0)

static void touch_file(const char *path)
{
    FILE *f = fopen(path, "w");
    if (f) fclose(f);
}

static int contains(const char *path, const char *text)
{
    char buffer[8192];
    size_t n;
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    n = fread(buffer, 1, sizeof(buffer) - 1, f);
    buffer[n] = '\0';
    fclose(f);
    return strstr(buffer, text) != NULL;
}

int main(int argc, char **argv)
{
    char home[512], path[1024], found[1024];
    const char *documents;
    const char *const folders[] = {"game", NULL};
    const char *const roms[] = {".sfc", ".smc", NULL};
    FILE *environment;
    int status = 0;
    pid_t child;
    if (argc < 2) {
        fprintf(stderr, "usage: test_ios SCRATCH_DIR\n");
        return 2;
    }
    snprintf(home, sizeof(home), "%s", argv[1]);
    mkdir(home, 0755);
    setenv("HOME", home, 1);
    /* An environment.txt from an earlier run. */
    snprintf(path, sizeof(path), "%s/Documents", home);
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/Documents/environment.txt", home);
    environment = fopen(path, "w");
    fputs("# a comment\nPORTEAME_TEST_SETTING=yes\nbroken line\n", environment);
    fclose(environment);

    child = fork();
    if (child == 0) {
        /* The startup sends stdout and stderr to the log: in a child. */
        documents = pe_ios_startup("Test", folders);
        if (!documents) _exit(10);
        printf("hello from the game\n");
        if (strcmp(getenv("PORTEAME_TEST_SETTING") ? getenv("PORTEAME_TEST_SETTING") : "", "yes")) _exit(11);
        if (strcmp(SDL_GetHint(SDL_HINT_ORIENTATIONS) ? SDL_GetHint(SDL_HINT_ORIENTATIONS) : "", "LandscapeLeft LandscapeRight")) _exit(12);
        if (!pe_ios_documents() || strcmp(pe_ios_documents(), documents)) _exit(13);
        return pe_ios_finish(7); /* exits the process, as on iOS */
    }
    waitpid(child, &status, 0);
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 7);
    snprintf(path, sizeof(path), "%s/Documents/port-eame-log.txt", home);
    CHECK(contains(path, "Port/eame Test: started"));
    CHECK(contains(path, "hello from the game"));
    CHECK(contains(path, "environment.txt sets PORTEAME_TEST_SETTING"));
    CHECK(contains(path, "Port/eame: closed (7)"));
    snprintf(path, sizeof(path), "%s/Documents/game", home);
    {
        struct stat info;
        CHECK(stat(path, &info) == 0 && S_ISDIR(info.st_mode));
    }
    /* A second run keeps the first one's log. */
    child = fork();
    if (child == 0) {
        pe_ios_startup("Test", folders);
        return pe_ios_finish(0);
    }
    waitpid(child, &status, 0);
    snprintf(path, sizeof(path), "%s/Documents/port-eame-log-previous.txt", home);
    CHECK(contains(path, "hello from the game"));

    /* Finding the game: any case, in Documents or one folder down, by name. */
    snprintf(path, sizeof(path), "%s/Documents", home);
    CHECK(!pe_ios_find_file(path, roms, found, sizeof(found)));
    snprintf(path, sizeof(path), "%s/Documents/game/EarthBound.SFC", home);
    touch_file(path);
    snprintf(path, sizeof(path), "%s/Documents/notes.txt", home);
    touch_file(path);
    snprintf(path, sizeof(path), "%s/Documents", home);
    CHECK(pe_ios_find_file(path, roms, found, sizeof(found)));
    CHECK(strstr(found, "/game/EarthBound.SFC") != NULL);
    snprintf(path, sizeof(path), "%s/Documents/A.smc", home);
    touch_file(path);
    snprintf(path, sizeof(path), "%s/Documents", home);
    CHECK(pe_ios_find_file(path, roms, found, sizeof(found)));
    CHECK(strstr(found, "/Documents/A.smc") != NULL);
    {
        /* A folder named like a ROM is not one. */
        const char *const bins[] = {".bin", NULL};
        snprintf(path, sizeof(path), "%s/Documents/disc.bin", home);
        mkdir(path, 0755);
        snprintf(path, sizeof(path), "%s/Documents", home);
        CHECK(!pe_ios_find_file(path, bins, found, sizeof(found)));
    }
    CHECK(pe_ios_window_flags() == (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS));
    if (failures) {
        fprintf(stderr, "%d failures\n", failures);
        return 1;
    }
    puts("ios: all good");
    return 0;
}
