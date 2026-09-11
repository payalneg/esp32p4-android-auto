/*
 * SPDX-License-Identifier: MIT
 * Copyright 2025 NXP
 */

/*********************
 *      INCLUDES
 *********************/
#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdlib.h>
#include <unistd.h>
#define SDL_MAIN_HANDLED        /*To fix SDL's "undefined reference to WinMain" issue*/
#include <SDL2/SDL.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/monitor.h"
#include "lv_drivers/indev/mouse.h"
#include "lv_drivers/indev/mousewheel.h"
#include "lv_drivers/indev/keyboard.h"
#include "gui_guider.h"
#include "custom.h"
#include "dashboard_theme.h"
#include "widgets_init.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#if LV_USE_FREEMASTER
#include "external_data_init.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void hal_init(void);
static int tick_thread(void * data);
static int shot_run(const char *screens, const char *out_dir);

/**********************
 *  STATIC VARIABLES
 **********************/
volatile int keep_running = 0;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

lv_ui guider_ui;

#if LV_USE_FREEMASTER
pthread_t thread[2];
pthread_mutex_t jsonrpc_mutex;
pthread_mutex_t lvgl_mutex;
pthread_mutex_t gg_edata_ll_mutex;
pthread_cond_t gg_edata_ll_cond;
#endif

int main(int argc, char ** argv)
{
#if LV_USE_FREEMASTER
    if(argc > 1)
    {
        int sim_conf(int argc, char *argv[]);
        sim_conf(argc, argv);
    }
#endif

    /* local threads will run */
    keep_running = 1;

    /*Initialize LittlevGL*/
    lv_init();

    /*Initialize the HAL (display, input devices, tick) for LittlevGL*/
    hal_init();

#if LV_USE_FREEMASTER
    pthread_mutex_init(&gg_edata_ll_mutex, NULL);
    pthread_cond_init(&gg_edata_ll_cond, NULL);
    /*Initialize the external data */
    external_task_init(&guider_ui);
#endif

    /*Create a GUI-Guider app */
    setup_ui(&guider_ui);
    custom_init(&guider_ui);

    /* Headless screenshot mode. With SDL_VIDEODRIVER=dummy this needs no
     * window and no display, which is what makes it usable over ssh, in CI,
     * and from an agent that has no screen-recording permission:
     *
     *   SDL_VIDEODRIVER=dummy SVD_SHOT_DIR=/tmp/shots SVD_SHOT=all \
     *       ./build480/bin/simulator
     *
     * SVD_SHOT is a comma-separated list of screens (theme names as they
     * appear in Settings, or "settings"), or "all". One BMP per screen; see
     * `make shots` / `make shots480`. */
    {
        const char *want = getenv("SVD_SHOT");
        if (want && *want) {
            const char *dir = getenv("SVD_SHOT_DIR");
            return shot_run(want, dir && *dir ? dir : ".");
        }
    }
#if LV_USE_FREEMASTER
    pthread_mutex_init(&jsonrpc_mutex, NULL);
    pthread_mutex_init(&lvgl_mutex, NULL);
    memset(&thread, 0, sizeof(thread));
    /*Create a separate thread to loop the linked list*/
    pthread_create(&thread[0], NULL, gg_edata_task_exec, &jsonrpc_mutex);
#endif

    while(keep_running) {
#if LV_USE_FREEMASTER
        pthread_mutex_lock(&lvgl_mutex);
        process_cached_widget_data();
#endif
        /* Periodically call the lv_task handler.
         * It could be done in a timer interrupt or an OS task too.*/
        lv_task_handler();
#if LV_USE_VIDEO
        video_play(&guider_ui);
#endif
#if LV_USE_FREEMASTER
        pthread_mutex_unlock(&lvgl_mutex);
#endif
        usleep(5 * 1000);
    }

#if LV_USE_FREEMASTER
    /* wakeup gg_edata_task_exec thread if it sleeps, let it finish */
    pthread_cond_signal(&gg_edata_ll_cond);
    /* wait for the gg_edata_task_exec thread*/
    pthread_join(thread[0], NULL);

    freemaster_disconnect();
#endif

    SDL_Quit();
    return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the Littlev graphics library
 */
static void hal_init(void)
{
    /* Create default group */
    lv_group_set_default(lv_group_create());

    /* Use the 'monitor' driver which creates window on PC's monitor to simulate a display*/
    monitor_init();

    /*Create a display buffer*/
    static lv_disp_draw_buf_t disp_buf1;
    static lv_color_t buf1_1[MONITOR_HOR_RES * MONITOR_VER_RES];
    lv_disp_draw_buf_init(&disp_buf1, buf1_1, NULL, MONITOR_HOR_RES * MONITOR_VER_RES);

    /*Create a display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
    disp_drv.draw_buf = &disp_buf1;
    disp_drv.flush_cb = monitor_flush;
    disp_drv.hor_res = MONITOR_HOR_RES;
    disp_drv.ver_res = MONITOR_VER_RES;
    lv_disp_drv_register(&disp_drv);

    /* Add the mouse as input device
     * Use the 'mouse' driver which reads the PC's mouse*/
    mouse_init();
    static lv_indev_drv_t mouse_indev_drv;
    lv_indev_drv_init(&mouse_indev_drv);          /*Basic initialization*/
    mouse_indev_drv.type = LV_INDEV_TYPE_POINTER;
    mouse_indev_drv.read_cb = mouse_read;         /*This function will be called periodically (by the library) to get the mouse position and state*/
    lv_indev_drv_register(&mouse_indev_drv);

#if USE_KEYBOARD
    /* Create default group */
    lv_group_set_default(lv_group_create());

    /* register keyboard device */
    static lv_indev_drv_t keyboard_indev_drv;
    lv_indev_drv_init(&keyboard_indev_drv);       /*Basic initialization*/
    keyboard_indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    keyboard_indev_drv.read_cb = keyboard_read;
    lv_indev_t *kb_indev = lv_indev_drv_register(&keyboard_indev_drv);
    lv_indev_set_group(kb_indev, lv_group_get_default());
#endif

    /* Tick init.
     * You have to call 'lv_tick_inc()' in periodically to inform LittelvGL about how much time were elapsed
     * Create an SDL thread to do this*/
    SDL_CreateThread(tick_thread, "tick", NULL);
}

/**
 * A task to measure the elapsed time for LittlevGL
 * @param data unused
 * @return never return
 */
static int tick_thread(void * data)
{
    (void)data;

    while(keep_running) {
        SDL_Delay(5);   /*Sleep for 5 millisecond*/
        lv_tick_inc(5); /*Tell LittelvGL that 5 milliseconds were elapsed*/
    }

    return 0;
}

#if LV_USE_FREEMASTER
/**
 * Enable simulator to be linked and used as DLL. This function is invoked by 
 * FreeMASTER from its background thread. This function must finish when 
 * sim_exit() is called asynchronously.
 */
__declspec(dllexport)
int sim_main(int argc, char *argv[])
{
    int ret = main(argc, argv);
    return ret;
}

/**
 * Finish all running threads and terminate the application running as sim_main().
 */
__declspec(dllexport)
int sim_exit(void)
{
    keep_running = 0;
    return 0;
}

/**
 * Enable FreeMASTER to configure the simulator before starting it.
 * This function is the first one called by the FreeMASTER from its GUI thread. 
 */
__declspec(dllexport)
int sim_conf(int argc, char *argv[])
{
    /* Process the arguments */
    if(argc > 0 && argv != NULL)
    {
        for(int i=0; i<argc; i++)
        {
            /* --rpcs is used to specify FreeMASTER JSON-RPC server address. */
            if(strcmp(argv[i], "--rpcs") == 0)
            {
                i++;
                if(i<argc && strncmp(argv[i], "ws://", 5) == 0)
                {
                    freemaster_server = strdup(argv[i]);
                }
            }
        }
    }
    return 0;
}

/**
 * Enable host application to retrieve DLL information
 */
__declspec(dllexport)
const void * sim_info(void)
{
    #define SIM_F_RESIZE_UP        0x0001   /* Enable growing size within parent view */
    #define SIM_F_RESIZE_DOWN      0x0002   /* Enable shrinking size within parent view */
    #define SIM_F_KEEP_ASPECT      0x0004   /* When RESIZE used, preserve aspect ratio */
    #define SIM_F_CENTER_H         0x0008   /* Center horizontally */
    #define SIM_F_CENTER_V         0x0010   /* Center vertically */
    #define SIM_F_CENTER_FORCE     0x0020   /* Keep centered even if parent is smaller (move beyond top/left border) */
    #define SIM_F_AUTO_BACKCOLOR   0x0040   /* Determine background color automatically */

    typedef struct
    {
        uint32_t sz;
        const char* name;
        const char* desc;
        const char* copyright;
        const char* server;
        uint32_t width;
        uint32_t height;
        uint32_t flags;
        uint32_t background;
        uint32_t reserved;
    } sim_info_t;

    /* Constant options: */
    static sim_info_t info =
    {
        .sz = sizeof(sim_info_t),
        .name = "NXP GUI-Guider Project",
        .desc = "Designed to enable GUI designs created in GG to be hosted in FreeMASTER",
        .copyright = "Copyright 2024 NXP",
        .server = "",
        .width = MONITOR_HOR_RES,
        .height = MONITOR_VER_RES,
        .flags = SIM_F_CENTER_H,
        .background = 0,
    };

    /* Runtime-defined options: */
    info.server = freemaster_server;

    return &info;
}

#endif

/*======================================================================
 * Headless screenshots
 *====================================================================*/

/* Let LVGL settle: run the task handler for `ms` of simulated time so timers,
 * screen-load animations and the first full render all complete. The tick
 * thread started by hal_init() is what advances lv_tick. */
static void shot_settle(int ms)
{
    for (int i = 0; i < ms / 5; i++) {
        lv_task_handler();
        usleep(5 * 1000);
    }
}

/* Write an LVGL snapshot as a 24-bit BMP — no image library, and every viewer
 * and `sips` on macOS reads it. LVGL gives us the screen in the configured
 * colour depth; both desktop configs are RGB565. */
static bool shot_write_bmp(const lv_img_dsc_t *dsc, const char *path)
{
    if (!dsc || !dsc->data) return false;
    const int w = dsc->header.w, h = dsc->header.h;
    const int row = (w * 3 + 3) & ~3;          /* BMP rows are 4-byte aligned */
    const int img_sz = row * h;

    FILE *f = fopen(path, "wb");
    if (!f) return false;

    uint8_t hdr[54] = {0};
    hdr[0] = 'B'; hdr[1] = 'M';
    uint32_t file_sz = 54 + img_sz;
    memcpy(hdr + 2, &file_sz, 4);
    uint32_t off = 54; memcpy(hdr + 10, &off, 4);
    uint32_t dib = 40;  memcpy(hdr + 14, &dib, 4);
    int32_t  iw = w;    memcpy(hdr + 18, &iw, 4);
    int32_t  ih = h;    memcpy(hdr + 22, &ih, 4);
    uint16_t planes = 1, bpp = 24;
    memcpy(hdr + 26, &planes, 2);
    memcpy(hdr + 28, &bpp, 2);
    uint32_t raw = img_sz; memcpy(hdr + 34, &raw, 4);
    fwrite(hdr, 1, sizeof hdr, f);

    uint8_t *line = calloc(1, row);
    if (!line) { fclose(f); return false; }
    for (int y = h - 1; y >= 0; y--) {          /* BMP is bottom-up */
        for (int x = 0; x < w; x++) {
            lv_color_t c = ((const lv_color_t *)dsc->data)[y * w + x];
            line[x * 3 + 0] = (uint8_t)(c.ch.blue  << 3);
            line[x * 3 + 1] = (uint8_t)(c.ch.green << 2);
            line[x * 3 + 2] = (uint8_t)(c.ch.red   << 3);
        }
        fwrite(line, 1, row, f);
    }
    free(line);
    fclose(f);
    return true;
}

static bool shot_one(const char *name, const char *out_dir)
{
    lv_obj_t *scr = NULL;

    if (strcmp(name, "settings") == 0) {
        setup_scr_settings(&guider_ui);
        scr = guider_ui.settings;
    } else {
        for (int i = 0; i < dashboard_theme_count(); i++) {
            const dashboard_theme_t *t = dashboard_theme_get(i);
            if (t && t->name && strcmp(t->name, name) == 0) {
                dashboard_theme_set(i);
                scr = dashboard_theme_active_screen();
                break;
            }
        }
        if (!scr) {
            fprintf(stderr, "shot: no screen or theme called \"%s\"\n", name);
            return false;
        }
    }

    lv_scr_load(scr);
    shot_settle(1200);

#if LV_USE_SNAPSHOT
    lv_img_dsc_t *dsc = lv_snapshot_take(lv_scr_act(), LV_IMG_CF_TRUE_COLOR);
    if (!dsc) {
        fprintf(stderr, "shot: lv_snapshot_take failed for \"%s\"\n", name);
        return false;
    }
    char path[512];
    char safe[128];
    size_t n = 0;
    for (const char *p = name; *p && n < sizeof(safe) - 1; p++) {
        safe[n++] = (*p == ' ' || *p == '/') ? '_' : *p;
    }
    safe[n] = 0;
    snprintf(path, sizeof path, "%s/%s.bmp", out_dir, safe);
    bool ok = shot_write_bmp(dsc, path);
    printf("%s %s (%dx%d)\n", ok ? "wrote" : "FAILED", path,
           (int)dsc->header.w, (int)dsc->header.h);
    lv_snapshot_free(dsc);
    return ok;
#else
    (void)out_dir;
    fprintf(stderr, "shot: built without LV_USE_SNAPSHOT — use `make shots`\n");
    return false;
#endif
}

static int shot_run(const char *screens, const char *out_dir)
{
    /* One settle pass before anything is asked for: custom_init() has just
     * built the default dashboard and its timers have not run yet. */
    shot_settle(600);

    int failures = 0;
    if (strcmp(screens, "all") == 0) {
        for (int i = 0; i < dashboard_theme_count(); i++) {
            const dashboard_theme_t *t = dashboard_theme_get(i);
            if (t && t->name && !shot_one(t->name, out_dir)) failures++;
        }
        if (!shot_one("settings", out_dir)) failures++;
    } else {
        char buf[512];
        strncpy(buf, screens, sizeof buf - 1);
        buf[sizeof buf - 1] = 0;
        for (char *tok = strtok(buf, ","); tok; tok = strtok(NULL, ",")) {
            while (*tok == ' ') tok++;
            if (*tok && !shot_one(tok, out_dir)) failures++;
        }
    }
    SDL_Quit();
    return failures ? 1 : 0;
}
