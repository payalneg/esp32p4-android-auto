/*
 * Headless driver for the simulator: screenshots, synthetic touch, and a
 * layout audit. See sim_drive.c for the script language.
 */
#pragma once

#include <stdbool.h>

/* Runs a script and returns a process exit code (0 = everything worked).
 * Called from main() instead of entering the SDL event loop when SVD_SHOT,
 * SVD_RUN or SVD_RUN_FILE is set in the environment. */
int sim_drive_run(const char *script, const char *out_dir);
