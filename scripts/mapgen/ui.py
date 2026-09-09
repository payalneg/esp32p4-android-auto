"""Тонкая tkinter-обёртка над MapEngine: эмуляция экрана устройства 800x480.

Все элементы управления — оверлеи поверх карты (как будет на ESP32-P4
с LVGL и тачем), окно фиксированного размера дисплея.
"""
import math
import tkinter as tk

from PIL import ImageDraw, ImageTk

import config
from map_engine import MapEngine
import profiles as prof
from navigator import BIKE_SPEED_MS, Navigator
from router import Router
from search import SearchIndex

RESULT_ZOOM = 17
ROUTE_COLOR = (30, 100, 220)
SEARCH_W = 300  # ширина поисковой строки и списка результатов

# Плашка манёвра наверху — как на автомобильных головных устройствах.
MANEUVER_GLYPH = {
    "turn_left": "↰", "turn_right": "↱",
    "slight_left": "↖", "slight_right": "↗",
    "sharp_left": "⬅", "sharp_right": "➡",
    "uturn": "⤺", "arrive": "⚑",
}


class MapUI:
    def __init__(self, engine=None):
        self.engine = engine or MapEngine()
        self.marker = config.START_CENTER  # Рыночная площадь по умолчанию
        self.index = SearchIndex()
        self._results: list[tuple[str, str, float, float]] = []
        self.router = Router()
        self.profile_var = None   # создаётся вместе с виджетами ниже
        self.route_start = None   # (lat, lon)
        self.route_end = None
        self.route = None         # результат router.route()
        self.route_message = ""
        self.nav = None        # ведение по маршруту (манёвры, остаток, сход)
        self.guide = None      # последнее состояние навигатора
        self.sim_along = 0.0   # положение в симуляции, метров от старта
        self.sim_job = None

        w, h = config.WINDOW_SIZE
        self.root = tk.Tk()
        self.root.title("Kraków Offline Map")
        self.root.geometry(f"{w}x{h}")
        self.root.resizable(False, False)  # эмулируем дисплей устройства

        # --- карта (весь экран) ---
        self.canvas = tk.Canvas(self.root, width=w, height=h,
                                highlightthickness=0, cursor="fleur")
        self.canvas.pack(fill=tk.BOTH, expand=True)
        self._photo = None  # держим ссылку, иначе tkinter отпустит изображение
        self._drag_start = None
        self._drag_dist = 0.0

        self.canvas.bind("<ButtonPress-1>", self._on_press)
        self.canvas.bind("<B1-Motion>", self._on_drag)
        self.canvas.bind("<ButtonRelease-1>", self._on_release)
        self.canvas.bind("<MouseWheel>", self._on_wheel)
        self.canvas.bind("<Double-Button-1>", self._on_double_click)
        self.canvas.bind("<Configure>", lambda e: self.redraw())
        # правый клик — точки веломаршрута (на macOS это Button-2)
        for seq in ("<Button-2>", "<Button-3>", "<Control-Button-1>"):
            self.canvas.bind(seq, self._on_route_click)
        self.root.bind("<Escape>", lambda e: self._on_escape())

        # --- оверлей поиска (левый верхний угол) ---
        self.query = tk.Entry(self.canvas, font=("Helvetica", 13))
        self.query.place(x=10, y=10, width=SEARCH_W - 64, height=30)
        self.query.bind("<Return>", lambda e: self._search())
        tk.Button(self.canvas, text="🔍", command=self._search).place(
            x=SEARCH_W - 50, y=10, width=40, height=30)
        self.results_box = tk.Listbox(self.canvas, font=("Helvetica", 11),
                                      height=6)
        self.results_box.bind("<<ListboxSelect>>", self._on_result)

        # --- профиль маршрутизации (правый верхний угол) ---
        self.profile_var = tk.StringVar(value=self.router.profile)
        profile_menu = tk.OptionMenu(self.canvas, self.profile_var,
                                     *prof.PROFILES, command=self._on_profile)
        profile_menu.configure(font=("Helvetica", 11), highlightthickness=0)
        profile_menu.place(relx=1.0, x=-10, y=10, anchor="ne", height=30)

        # --- кнопки зума (правый нижний угол, «тач»-размер) ---
        for text, dz, dy in (("+", 1, -104), ("−", -1, -60)):
            tk.Button(self.canvas, text=text, font=("Helvetica", 16),
                      command=lambda d=dz: self._zoom_center(d)).place(
                relx=1.0, rely=1.0, x=-10, y=dy, width=40, height=40,
                anchor="ne")

        # --- режим маршрута для тачскрина: кнопка вместо правого клика ---
        self.route_mode = False
        self.route_btn = tk.Button(self.canvas, text="🚲", font=("Helvetica", 16),
                                   command=self._toggle_route_mode)
        self.route_btn.place(relx=1.0, rely=1.0, x=-10, y=-148,
                             width=40, height=40, anchor="ne")

        # --- симуляция поездки: ведёт позицию по маршруту без GPS ---
        self.sim_btn = tk.Button(self.canvas, text="▶", font=("Helvetica", 16),
                                 command=self._toggle_sim)
        self.sim_btn.place(relx=1.0, rely=1.0, x=-10, y=-192,
                           width=40, height=40, anchor="ne")

    # --- поиск ---

    def _search(self):
        try:
            self._results = self.index.search(self.query.get())
        except FileNotFoundError:
            self._results = []
            self._show_results(["Индекс не собран: main.py --build"])
            return
        if not self._results:
            self._show_results(["Ничего не найдено"])
            return
        self._show_results([f"{d}  ({k})" for d, k, _, _ in self._results])

    def _show_results(self, lines):
        self.results_box.delete(0, tk.END)
        for line in lines:
            self.results_box.insert(tk.END, line)
        self.results_box.place(x=10, y=44, width=SEARCH_W - 10)

    def _hide_results(self):
        self.results_box.place_forget()

    def _on_result(self, _event):
        sel = self.results_box.curselection()
        if not sel or not self._results:
            return
        _, _, lat, lon = self._results[sel[0]]
        self.engine.center_lat, self.engine.center_lon = lat, lon
        self.engine.zoom = min(RESULT_ZOOM, self.engine.ZOOM_LIMITS[1])
        self.marker = (lat, lon)
        self._hide_results()
        self.redraw()

    # --- веломаршрут ---

    def _toggle_route_mode(self):
        """Тач-режим: следующие тапы по карте ставят старт и финиш."""
        self.route_mode = not self.route_mode
        self.route_btn.configure(relief=tk.SUNKEN if self.route_mode else tk.RAISED)
        if self.route_mode:
            self._reset_route()
            self.route_message = "старт — тап по карте"
        else:
            self.route_message = ""
        self.redraw()

    def _on_profile(self, name):
        """Смена профиля: граф не перечитывается, цена рёбер считается заново."""
        self.router.set_profile(name)
        if self.route_start and self.route_end:
            self.route_message = "пересчитываю..."
            self.redraw()
            self.root.update_idletasks()
            self._recalc_route()
        self.redraw()

    def _recalc_route(self):
        """Строит маршрут по текущим точкам и поднимает навигатор."""
        self._stop_sim()
        try:
            self.route = self.router.route(self.route_start, self.route_end)
        except FileNotFoundError:
            self.route, self.route_message = None, "Граф не собран: main.py --build"
            self.nav = self.guide = None
            return
        if self.route:
            self.nav = Navigator(self.route)
            self.sim_along = 0.0
            self.guide = self.nav.update(*self.route_start)
            self.route_message = ""
        else:
            self.nav = self.guide = None
            self.route_message = "Маршрут не найден"

    def _on_route_click(self, event):
        lat, lon = self.engine.screen_to_geo(event.x, event.y,
                                             self.canvas.winfo_width(),
                                             self.canvas.winfo_height())
        if self.route_start is None or self.route_end is not None:
            # новая пара точек
            self.route_start, self.route_end = (lat, lon), None
            self.route, self.route_message = None, "теперь укажите финиш"
        else:
            self.route_end = (lat, lon)
            self.route_message = "строю маршрут..."
            self.redraw()
            self.root.update_idletasks()
            self._recalc_route()
        self.redraw()

    def _reset_route(self):
        self._stop_sim()
        self.route_start = self.route_end = self.route = None
        self.nav = self.guide = None
        self.route_message = ""
        self.redraw()

    # --- симуляция поездки ---

    SIM_TICK_MS = 200
    SIM_SPEEDUP = 3     # втрое быстрее велосипеда, иначе смотреть долго

    def _toggle_sim(self):
        if self.sim_job is not None:
            self._stop_sim()
            self.redraw()
            return
        if self.nav is None:
            self.route_message = "сначала постройте маршрут"
            self.redraw()
            return
        self.sim_along = 0.0
        self.sim_btn.configure(relief=tk.SUNKEN)
        self._sim_step()

    def _stop_sim(self):
        if self.sim_job is not None:
            self.root.after_cancel(self.sim_job)
            self.sim_job = None
        self.sim_btn.configure(relief=tk.RAISED)

    def _sim_step(self):
        self.sim_along += BIKE_SPEED_MS * self.SIM_TICK_MS / 1000 * self.SIM_SPEEDUP
        lat, lon = self.nav.position_at(self.sim_along)
        self.guide = self.nav.update(lat, lon)
        self.engine.center_lat, self.engine.center_lon = lat, lon   # карта едет за позицией
        self.redraw()
        if self.guide["arrived"]:
            self._stop_sim()
            return
        self.sim_job = self.root.after(self.SIM_TICK_MS, self._sim_step)

    def _on_escape(self):
        self._hide_results()
        self._reset_route()

    # --- события мыши ---

    def _on_press(self, event):
        self._drag_start = (event.x, event.y)
        self._drag_dist = 0.0

    def _on_drag(self, event):
        if self._drag_start is None:
            return
        dx = self._drag_start[0] - event.x
        dy = self._drag_start[1] - event.y
        self._drag_dist += abs(dx) + abs(dy)
        self._drag_start = (event.x, event.y)
        self.engine.pan(dx, dy)
        self.redraw()

    def _on_release(self, event):
        # короткий тап (не пан) в режиме маршрута = точка маршрута
        if self.route_mode and self._drag_dist < 6:
            self._on_route_click(event)
        self._drag_start = None

    def _on_wheel(self, event):
        delta = 1 if event.delta > 0 else -1
        if self.engine.zoom_at(event.x, event.y, delta,
                               self.canvas.winfo_width(),
                               self.canvas.winfo_height()):
            self.redraw()

    def _on_double_click(self, event):
        if self.engine.zoom_at(event.x, event.y, 1,
                               self.canvas.winfo_width(),
                               self.canvas.winfo_height()):
            self.redraw()

    def _zoom_center(self, dz):
        if self.engine.zoom_at(self.canvas.winfo_width() / 2,
                               self.canvas.winfo_height() / 2, dz,
                               self.canvas.winfo_width(),
                               self.canvas.winfo_height()):
            self.redraw()

    # --- отрисовка ---

    def redraw(self):
        w = max(self.canvas.winfo_width(), 1)
        h = max(self.canvas.winfo_height(), 1)
        frame = self.engine.render(w, h)
        self._draw_route(frame, w, h)
        self._draw_marker(frame, w, h)
        self._photo = ImageTk.PhotoImage(frame)
        self.canvas.delete("all")
        self.canvas.create_image(0, 0, image=self._photo, anchor=tk.NW)
        self._draw_scale_bar(h)
        self._draw_maneuver(w)
        self._draw_route_info(w, h)
        self.root.title(
            f"Kraków Offline Map — z{self.engine.zoom} "
            f"({self.engine.center_lat:.4f}, {self.engine.center_lon:.4f})"
        )

    def _draw_route(self, frame, w, h):
        draw = ImageDraw.Draw(frame)
        if self.route:
            pts = [self.engine.geo_to_screen(lat, lon, w, h)
                   for lat, lon in self.route["points"]]
            draw.line(pts, fill=(255, 255, 255), width=9, joint="curve")
            draw.line(pts, fill=ROUTE_COLOR, width=5, joint="curve")
        for point, color in ((self.route_start, (40, 160, 60)),
                             (self.route_end, (210, 60, 40))):
            if point is None:
                continue
            x, y = self.engine.geo_to_screen(*point, w, h)
            r = 7
            draw.ellipse((x - r, y - r, x + r, y + r), fill=color,
                         outline=(255, 255, 255), width=2)
        if self.nav is not None and self.sim_job is not None:
            x, y = self.engine.geo_to_screen(
                *self.nav.position_at(self.sim_along), w, h)
            r = 8
            draw.ellipse((x - r, y - r, x + r, y + r), fill=(20, 120, 240),
                         outline=(255, 255, 255), width=3)

    def _draw_maneuver(self, w):
        """Плашка следующего манёвра наверху экрана."""
        if not self.guide:
            return
        if self.guide["off_route"]:
            text, bg = "сход с маршрута", "#c0392b"
        else:
            nxt = self.guide["next"]
            d = nxt["dist_m"]
            dist = f"{d:.0f} м" if d < 1000 else f"{d / 1000:.1f} км"
            text = f"{MANEUVER_GLYPH.get(nxt['type'], '•')}  {dist}"
            bg = "#333333"
        self.canvas.create_rectangle(w / 2 - 90, 8, w / 2 + 90, 46,
                                     fill=bg, outline="")
        self.canvas.create_text(w / 2, 27, text=text, fill="white",
                                font=("Helvetica", 18, "bold"))

    def _draw_route_info(self, w, h):
        if self.guide:
            km = self.guide["remaining_m"] / 1000
            mins = max(1, round(self.guide["remaining_s"] / 60))
            text = (f"🚲 {km:.1f} км · ~{mins} мин"
                    f" · тротуары {self._foot_share():.0f}%")
            self.canvas.create_rectangle(w / 2 - 140, h - 40, w / 2 + 140, h - 14,
                                         fill="white", outline="#bbb")
            self.canvas.create_text(w / 2, h - 27, text=text,
                                    font=("Helvetica", 11))
            return
        if self.route:
            km = self.route["length_m"] / 1000
            mins = max(1, round(self.route["time_s"] / 60))
            text = f"🚲 {km:.1f} км · ~{mins} мин  (Esc — сброс)"
        elif self.route_message:
            text = self.route_message
        else:
            return
        self.canvas.create_rectangle(w / 2 - 140, h - 40, w / 2 + 140, h - 14,
                                     fill="white", outline="#bbb")
        self.canvas.create_text(w / 2, h - 27, text=text,
                                font=("Helvetica", 11))

    def _foot_share(self) -> float:
        """Доля маршрута по тротуарам и пешеходным дорожкам, %."""
        if not self.route or not self.route.get("legs"):
            return 0.0
        foot = {prof.CLASS_INDEX[c] for c in prof.FOOT_CLASSES}
        total = sum(dm for _, dm in self.route["legs"])
        if not total:
            return 0.0
        return 100.0 * sum(dm for cls, dm in self.route["legs"]
                           if cls in foot) / total

    def _draw_marker(self, frame, w, h):
        mx, my = self.engine.geo_to_screen(*self.marker, w, h)
        if not (-20 <= mx <= w + 20 and -20 <= my <= h + 20):
            return
        draw = ImageDraw.Draw(frame)
        r = 7
        draw.line((mx, my, mx, my - 22), fill=(200, 30, 30), width=3)
        draw.ellipse((mx - r, my - 22 - r, mx + r, my - 22 + r),
                     fill=(220, 50, 50), outline=(120, 10, 10), width=2)

    def _draw_scale_bar(self, h):
        """Масштабная линейка внизу слева (метры на текущем зуме и широте)."""
        mpp = (40075016.686 * math.cos(math.radians(self.engine.center_lat))
               / (config.TILE_SIZE * 2 ** self.engine.zoom))
        meters = 10
        for m in (50000, 20000, 10000, 5000, 2000, 1000, 500, 200, 100, 50, 20, 10):
            if m / mpp <= 100:
                meters = m
                break
        px = meters / mpp
        label = f"{meters // 1000} км" if meters >= 1000 else f"{meters} м"
        x0, y0 = 10, h - 14
        self.canvas.create_rectangle(x0 - 5, y0 - 22, x0 + px + 7, y0 + 8,
                                     fill="white", outline="#bbb")
        self.canvas.create_line(x0, y0, x0 + px, y0, width=2)
        self.canvas.create_line(x0, y0 - 4, x0, y0 + 4, width=2)
        self.canvas.create_line(x0 + px, y0 - 4, x0 + px, y0 + 4, width=2)
        self.canvas.create_text(x0 + px / 2, y0 - 11, text=label,
                                font=("Helvetica", 10))

    def run(self):
        self.root.after(50, self.redraw)
        self.root.mainloop()
