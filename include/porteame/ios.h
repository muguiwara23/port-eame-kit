#ifndef PORTEAME_IOS_H
#define PORTEAME_IOS_H
/* Port/eame kit: what every Port/eame iOS app does the same way (the
 * contract, docs/es/contrato.md), as small SDL3 helpers. Off iOS each one
 * does nothing or the plain thing, so a game calls them without #ifdefs.
 *
 *     int main(int argc, char **argv)
 *     {
 *         const char *const folders[] = {"game", NULL};
 *         pe_ios_startup("EarthBound", folders);  // log, Documents, landscape
 *         ...create the window with pe_ios_window_flags()...
 *         while (running) {
 *             while (SDL_PollEvent(&event)) {
 *                 if (pe_ios_is_background(&event)) {
 *                     if (!pe_ios_wait_for_foreground(save_everything, game)) running = 0;
 *                     continue;
 *                 }
 *                 ...
 *             }
 *         }
 *         ...save, destroy, SDL_Quit()...
 *         return pe_ios_finish(0);  // on iOS: really ends the app
 *     }
 *
 * Main thread only. */
#include <SDL3/SDL.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* First thing in main, before SDL_Init. On iOS:
 *  - makes the app's Documents folder (the one the Files app shows) and the
 *    given subfolders in it (a NULL-terminated list, or NULL), so the player
 *    sees where the game goes;
 *  - sends stdout and stderr, unbuffered, to Documents/port-eame-log.txt,
 *    keeping the last run's as port-eame-log-previous.txt;
 *  - reads Documents/environment.txt, NAME=value lines, into the
 *    environment (to try a setting on a device with no shell);
 *  - asks SDL for landscape, left or right: Info.plist's orientations are
 *    not read when another app hosts this one (LiveContainer).
 * Returns the Documents folder with a trailing '/', or NULL off iOS. */
const char *pe_ios_startup(const char *app_name, const char *const *folders);

/* The Documents folder with a trailing '/' on iOS, NULL elsewhere. */
const char *pe_ios_documents(void);

/* The first file (by name) in `folder` or one of its subfolders whose
 * extension is one of `extensions` (NULL-terminated, like ".sfc", any case).
 * Writes its path into `out`; 0 when there is none. Works on any system. */
int pe_ios_find_file(const char *folder, const char *const *extensions, char *out, size_t size);

/* The window flags a Port/eame app adds on iOS: full screen, borderless. 0
 * elsewhere. */
SDL_WindowFlags pe_ios_window_flags(void);

/* Pixels per point of the window (2 or 3 on a phone; 1 if unknown). SDL's
 * window sizes, mouse and safe area are in points on iOS. */
float pe_ios_density(SDL_Window *window);

/* The safe area's insets from the left, right and bottom edges, in pixels:
 * what keeps the controls clear of the notch, the Dynamic Island and the
 * home bar. All 0 when SDL does not know. */
void pe_ios_safe_area(SDL_Window *window, int *left, int *right, int *bottom);

/* Whether an event says the app is going to the background. */
int pe_ios_is_background(const SDL_Event *event);

/* The app went to the background: iOS refuses GPU work there, so the game
 * and its drawing must wait. Calls `save` (if any) first, since iOS may end
 * the app without another word, then waits until the app is back in front.
 * 0 when the app is closed meanwhile. */
int pe_ios_wait_for_foreground(void (*save)(void *userdata), void *userdata);

/* An error the player must see: a message box (on iOS, where nothing else
 * would show), also written to stderr, so the log has it. */
void pe_ios_error(const char *title, const char *message);

/* The end of main: `return pe_ios_finish(status);`. On iOS SDL3 does not end
 * the app when main returns (it keeps UIKit's run loop going), which leaves a
 * black screen; this flushes the output and ends the process. Elsewhere it
 * returns `status`. */
int pe_ios_finish(int status);

#ifdef __cplusplus
}
#endif
#endif
