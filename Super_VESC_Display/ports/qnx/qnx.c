/*
* Copyright 2023 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "qnx.h"

#define SCREEN_BUFFER_COUNT (1)
#define SCREEN_DIRTY_RECTS_MAX (128)

typedef struct _qnx_window
{
    /* Screen content */
    screen_context_t screen_ctx;
    screen_window_t screen_win;
    screen_event_t screen_ev;
    void *screen_wbuf_pointer;
    int screen_wbuf_stride;
    screen_buffer_t screen_wbuf[SCREEN_BUFFER_COUNT]; /* renderable buffers for the window */
    int dirty_rects[SCREEN_DIRTY_RECTS_MAX * 4];
    unsigned dirty_rects_cnt;

    /* LVGL content */
    void *lv_wbuf_pointer;
    lv_disp_t *lv_disp;
    lv_disp_draw_buf_t disp_buf;
    lv_disp_drv_t lv_disp_drv;
    lv_indev_drv_t lv_indev_drv_pointer;
    lv_indev_t *lv_indev_pointer;

    /* Settings */
    unsigned screen_format;
    int screen_size[2];

} qnx_window_t;

static void qnx_screen_flush(struct _lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);
static int qnx_destroy_window(qnx_window_t *window);
static void qnx_pointer_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

/* PUBLIC API */
void lv_qnx_init(void)
{
    /* Nothing there */
}

lv_disp_t *lv_qnx_create_window(lv_coord_t hor_res, lv_coord_t ver_res, const char *name)
{
    int status = -1;
    int transp = SCREEN_TRANSPARENCY_NONE;
    int zorder = 0;
    size_t area = (size_t)hor_res * ver_res;
    size_t size = area * sizeof(lv_color_t);

    qnx_window_t *window = NULL;

    window = lv_mem_alloc(sizeof(*window));
    LV_ASSERT_MALLOC(window);
    if (!window)
    {
        perror("lv_mem_alloc failed");
        goto fail;
    }

#if LV_COLOR_DEPTH == 16
    window->screen_format = SCREEN_FORMAT_RGB565;
#elif LV_COLOR_DEPTH == 32
    window->screen_format = SCREEN_FORMAT_RGBA8888;
#else
#error Unsupported LV_COLOR_DEPTH.
#endif

    if (screen_create_context(&window->screen_ctx, 0))
    {
        perror("screen_context_create");
        goto fail;
    }
    if (screen_create_window(&window->screen_win, window->screen_ctx))
    {
        perror("screen_create_window");
        goto fail;
    }

    status = screen_set_window_property_iv(window->screen_win, SCREEN_PROPERTY_FORMAT, (const int[]){window->screen_format});
    if (status)
    {
        perror("error SCREEN_PROPERTY_FORMAT");
        goto fail;
    }

    window->screen_size[0] = hor_res;
    window->screen_size[1] = ver_res;
    status = screen_set_window_property_iv(window->screen_win, SCREEN_PROPERTY_SIZE, window->screen_size);
    if (status)
    {
        perror("error SCREEN_PROPERTY_SIZE");
        goto fail;
    }

    status = screen_set_window_property_iv(window->screen_win, SCREEN_PROPERTY_TRANSPARENCY, &transp);
    if (status)
    {
        perror("screen_set_window_property_iv(SCREEN_PROPERTY_TRANSPARENCY)");
        goto fail;
    }

    status = screen_set_window_property_iv(window->screen_win, SCREEN_PROPERTY_ZORDER, &zorder);
    if (status)
    {
        perror("screen_set_window_property_iv(SCREEN_PROPERTY_ZORDER)");
        goto fail;
    }

    (void)screen_set_window_property_cv(window->screen_win, SCREEN_PROPERTY_ID_STRING, strlen(name), name);

    if (screen_create_window_buffers(window->screen_win, SCREEN_BUFFER_COUNT))
    {
        perror("screen_create_window_buffers() failed");
        goto fail;
    }
    status = screen_get_window_property_pv(window->screen_win, SCREEN_PROPERTY_RENDER_BUFFERS, (void **)&window->screen_wbuf);
    if (status)
    {
        perror("screen_get_window_property_pv(SCREEN_PROPERTY_RENDER_BUFFERS) failed");
        goto fail;
    }

    status = screen_get_buffer_property_pv(window->screen_wbuf[0], SCREEN_PROPERTY_POINTER, &window->screen_wbuf_pointer);
    if (status)
    {
        perror("screen_get_buffer_property_pv(SCREEN_PROPERTY_POINTER)");
        goto fail;
    }
    status = screen_get_buffer_property_iv(window->screen_wbuf[0], SCREEN_PROPERTY_STRIDE, (void *)&window->screen_wbuf_stride);
    if (status)
    {
        perror("screen_get_buffer_property_iv(SCREEN_PROPERTY_STRIDE)");
        goto fail;
    }
    /* Stride is in bytes, make it in px */
    window->screen_wbuf_stride /= LV_COLOR_DEPTH >> 3;
    if (window->screen_wbuf_stride < hor_res)
    {
        printf("Unexpected stride %i\n", window->screen_wbuf_stride);
        goto fail;
    }
#if !LV_MEM_CUSTOM
#warning Performance might be degraded. Use LV_MEM_CUSTOM.
    if (size > LV_MEM_SIZE)
    {
        /* LVGL could draw to buffer with size smaller then the screen size */
        printf("Warning: performance might be degraded!\n");
        area >>= 3;
        size >>= 3;
    }
#endif
    /* Initialize draw buffer for LVGL */
    window->lv_wbuf_pointer = lv_mem_alloc(size);

    if (!window->lv_wbuf_pointer)
    {
#if !LV_MEM_CUSTOM
        perror("Cannot allocate lv_wbuf_pointer. Check LV_MEM_SIZE.");
#else
        perror("Cannot allocate lv_wbuf_pointer");
#endif
        goto fail;
    }

    lv_disp_draw_buf_init(&window->disp_buf, window->lv_wbuf_pointer, NULL, area);
    lv_disp_drv_init(&window->lv_disp_drv);

    window->lv_disp_drv.draw_buf = &window->disp_buf;
    window->lv_disp_drv.hor_res = hor_res;
    window->lv_disp_drv.ver_res = ver_res;
    window->lv_disp_drv.flush_cb = qnx_screen_flush;
    window->lv_disp_drv.user_data = window;

    window->lv_disp = lv_disp_drv_register(&window->lv_disp_drv);

    if (!window->lv_disp)
    {
        perror("lv_disp_drv_register failed.");
        goto fail;
    }

    status = screen_create_event(&window->screen_ev);
    if (status)
    {
        perror("screen_create_event");
        goto fail;
    }

    /* Register input */
    lv_indev_drv_init(&window->lv_indev_drv_pointer);
    window->lv_indev_drv_pointer.type = LV_INDEV_TYPE_POINTER;
    window->lv_indev_drv_pointer.read_cb = qnx_pointer_read;
    window->lv_indev_drv_pointer.disp = window->lv_disp;
    window->lv_indev_pointer = lv_indev_drv_register(&window->lv_indev_drv_pointer);
    if (!window->lv_indev_pointer)
    {
        perror("failed to register pointer indev");
        goto fail;
    }

    return window->lv_disp;
fail:
    qnx_destroy_window(window);
    return NULL;
}

int lv_qnx_destroy_window(lv_disp_t *disp)
{
    LV_ASSERT(disp);
    LV_ASSERT(disp->driver->user_data);
    return qnx_destroy_window(disp->driver->user_data);
}

static void qnx_pointer_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    int status;
    int val;
    static int pos[2];
    static int pressed = 0;
    qnx_window_t *window;

    LV_ASSERT(drv && drv->disp && drv->disp->driver);
    window = drv->disp->driver->user_data;
    LV_ASSERT(window);

    data->point.x = 0;
    data->point.y = 0;
    data->state = 0;

    while (!screen_get_event(window->screen_ctx, window->screen_ev, 0))
    {
        status = screen_get_event_property_iv(window->screen_ev, SCREEN_PROPERTY_TYPE, &val);
        if (status || val == SCREEN_EVENT_NONE)
        {
            return;
        }
        switch (val)
        {
        case SCREEN_EVENT_POINTER:
            screen_get_event_property_iv(window->screen_ev, SCREEN_PROPERTY_BUTTONS, &val);
            if (val)
            {
                screen_get_event_property_iv(window->screen_ev, SCREEN_PROPERTY_POSITION, pos);
                data->point.x = pos[0];
                data->point.y = pos[1];
                data->state = LV_INDEV_STATE_PRESSED;
                pressed = 1;
            }
            else
            {
                if (pressed)
                {
                    data->point.x = pos[0];
                    data->point.y = pos[1];
                    data->state = LV_INDEV_STATE_RELEASED;
                    pressed = 0;
                }
            }
            break;
        }
    }
}

static int qnx_destroy_window(qnx_window_t *window)
{
    int status;
    int ret = 0;
    if (window)
    {
        if (window->lv_disp)
        {
            lv_disp_remove(window->lv_disp);
        }
        if (window->lv_wbuf_pointer)
        {
            lv_mem_free(window->lv_wbuf_pointer);
        }
        if (window->screen_win)
        {
            if (window->screen_wbuf)
            {
                status = screen_destroy_window_buffers(window->screen_win);
                if (status)
                {
                    perror("screen_destroy_window_buffers failed");
                    ret = -1;
                }
            }
            status = screen_destroy_window(window->screen_win);
            if (status)
            {
                perror("screen_destroy_window failed");
                ret = -1;
            }
        }
        if (window->screen_ev)
        {
            status = screen_destroy_event(window->screen_ev);
            if (status)
            {
                perror("screen_destroy_event failed.");
                ret = -1;
            }
        }
        if (window->screen_ctx)
        {
            status = screen_destroy_context(window->screen_ctx);
            if (status)
            {
                perror("screen_destroy_context failed.");
                ret = -1;
            }
        }
        lv_mem_free(window);
    }
    return ret;
}

static void qnx_screen_flush(struct _lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    int status;
    int *p_dirty = NULL;
    int count = 0;
    int32_t x;
    int32_t y;
    qnx_window_t *window = disp_drv->user_data;
    LV_ASSERT(window);

    /* Manage dirty rectangles up to SCREEN_DIRTY_RECTS_MAX otherwise refresh complete screen */
    if (window->dirty_rects_cnt < SCREEN_DIRTY_RECTS_MAX)
    {
        window->dirty_rects[0 + (4 * window->dirty_rects_cnt)] = area->x1;
        window->dirty_rects[1 + (4 * window->dirty_rects_cnt)] = area->y1;
        window->dirty_rects[2 + (4 * window->dirty_rects_cnt)] = area->x2;
        window->dirty_rects[3 + (4 * window->dirty_rects_cnt++)] = area->y2;
        p_dirty = window->dirty_rects;
        count = window->dirty_rects_cnt;
    }

    /* Update data in screen buffer */
    for (y = area->y1; y <= area->y2 && y < disp_drv->ver_res; y++)
    {
        for (x = area->x1; x <= area->x2 && x < disp_drv->hor_res; x++)
        {
            int offset = (y * window->screen_wbuf_stride) + x;
            lv_color_t *const buf = (lv_color_t *)window->screen_wbuf_pointer + offset;
            buf->full = color_p->full;
            color_p++;
        }
    }

    if (lv_disp_flush_is_last(disp_drv))
    {
        status = screen_post_window(window->screen_win, window->screen_wbuf[0], count, p_dirty, SCREEN_WAIT_IDLE);
        if (status)
        {
            perror("error screen_post_window");
        }
        window->dirty_rects_cnt = 0;
    }
    lv_disp_flush_ready(disp_drv);
}
