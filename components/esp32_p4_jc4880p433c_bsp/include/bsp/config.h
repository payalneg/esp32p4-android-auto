/*
 * Minimal BSP config for JC4880P4 board
 */

#pragma once

// By default, this BSP is shipped with LVGL graphical library. Enabling this option will exclude it.
// If you want to use BSP without LVGL, select BSP version with 'noglib' suffix.
#if !defined(BSP_CONFIG_NO_GRAPHIC_LIB)
#define BSP_CONFIG_NO_GRAPHIC_LIB (0)
#endif
