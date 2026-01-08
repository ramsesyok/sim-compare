import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.backends._backend_tk import NavigationToolbar2Tk
import geopandas as gpd
import json
import pandas as pd
import numpy as np
import sys # 強制終了用にインポート

# --- 定数・設定 ---
MAP_FILE_PATH = "./map_data/ne_110m_coastline.shp"
DEFAULT_FILENAME = "timeline.log"

# 表示スタイル設定
STYLES = {
    "A": {"color": "blue"},
    "B": {"color": "red"},
    "commander": {"marker": "*", "s": 100, "label": "Cmd"},
    "scout":     {"marker": "^", "s": 30,  "label": "Scout"},
    "messenger": {"marker": "s", "s": 30,  "label": "Msg"},
    "attacker":  {"marker": "x", "s": 40,  "label": "Atk"},
}

class LogLoader:
    """ログファイルを読み込み、軽量な形式で保持するクラス"""
    def __init__(self):
        self.data = {} # key: time_sec, value: list of dicts (Not DataFrame)
        self.max_time = 0
        self.min_time = 0
        self.loaded = False

    def load_file(self, filepath):
        self.data = {}
        
        try:
            # メモリ節約のため、DataFrame化せずそのままリストとして保持
            with open(filepath, 'r', encoding='utf-8') as f:
                for line in f:
                    if not line.strip(): continue
                    row = json.loads(line)
                    t = row["time_sec"]
                    self.data[t] = row["positions"]
            
            if not self.data:
                raise ValueError("有効なデータがありません")

            times = sorted(self.data.keys())
            self.min_time = times[0]
            self.max_time = times[-1]
            self.loaded = True
            return True, f"Loaded {len(times)} time steps.\nRange: {self.min_time}s - {self.max_time}s"

        except Exception as e:
            return False, str(e)

    def get_positions_at(self, time_sec):
        """指定時間のデータをDataFrameに変換して返す"""
        raw_data = self.data.get(time_sec)
        if raw_data:
            # 必要な瞬間だけDataFrame化する（1000行程度なら一瞬）
            return pd.DataFrame(raw_data)
        return None

class ViewerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Simulation Log Viewer (Optimized)")
        self.root.geometry("1200x850")
        
        # 終了時の処理を上書き (強制終了)
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        
        self.loader = LogLoader()
        self.map_data = None
        self.is_playing = False
        self.play_job = None
        
        # マウス操作用
        self.pan_start = None

        # 描画用アーティストのキャッシュ
        self.artists = {} 

        self._init_gui()
        self._load_map_data()

    def on_closing(self):
        """ウィンドウが閉じられたときにプロセスを強制終了する"""
        self.root.destroy()
        sys.exit(0) # ガーベージコレクションを待たずに終了

    def _init_gui(self):
        # --- コントロールパネル (上部) ---
        top_frame = ttk.Frame(self.root, padding="5")
        top_frame.pack(side=tk.TOP, fill=tk.X)

        ttk.Button(top_frame, text="Load Log...", command=self.load_log).pack(side=tk.LEFT, padx=5)
        self.lbl_status = ttk.Label(top_frame, text="No log loaded", foreground="gray")
        self.lbl_status.pack(side=tk.LEFT, padx=10)

        # 再生コントロール
        control_frame = ttk.Frame(top_frame)
        control_frame.pack(side=tk.RIGHT, padx=5)

        ttk.Label(control_frame, text="Step Interval:").pack(side=tk.LEFT)
        self.var_step = tk.StringVar(value="60") # デフォルトを60秒に変更(長時間のログのため)
        ttk.Entry(control_frame, textvariable=self.var_step, width=5).pack(side=tk.LEFT, padx=2)
        ttk.Label(control_frame, text="sec").pack(side=tk.LEFT, padx=(0, 10))

        self.btn_play = ttk.Button(control_frame, text="▶ Play", command=self.toggle_play, state=tk.DISABLED)
        self.btn_play.pack(side=tk.LEFT, padx=2)
        
        # --- スライダーパネル ---
        slider_frame = ttk.Frame(self.root, padding="5")
        slider_frame.pack(side=tk.TOP, fill=tk.X)
        
        self.var_time = tk.IntVar(value=0)
        self.slider = ttk.Scale(slider_frame, from_=0, to=100, variable=self.var_time, orient=tk.HORIZONTAL, command=self.on_slider_move)
        self.slider.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)
        self.slider.state(["disabled"])

        self.lbl_time = ttk.Label(slider_frame, text="Time: 00:00:00 (0s)", font=("Consolas", 12))
        self.lbl_time.pack(side=tk.RIGHT, padx=10)

        # --- メイン描画エリア ---
        plot_frame = ttk.Frame(self.root)
        plot_frame.pack(fill=tk.BOTH, expand=True)

        self.fig, self.ax = plt.subplots(figsize=(8, 8))
        self.canvas = FigureCanvasTkAgg(self.fig, master=plot_frame)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        toolbar = NavigationToolbar2Tk(self.canvas, plot_frame)
        toolbar.update()

        self.canvas.mpl_connect("scroll_event", self.on_scroll)
        self.canvas.mpl_connect("button_press_event", self.on_press)
        self.canvas.mpl_connect("motion_notify_event", self.on_motion)
        self.canvas.mpl_connect("button_release_event", self.on_release)

    def _load_map_data(self):
        try:
            self.map_data = gpd.read_file(MAP_FILE_PATH)
            self._draw_map_base()
        except Exception as e:
            self.map_data = None
            self.ax.grid(True)
            self.ax.set_title("No Map Data Loaded")

    def _draw_map_base(self):
        self.ax.clear()
        if self.map_data is not None:
            self.map_data.plot(ax=self.ax, color='#f0f0f0', edgecolor='#bbbbbb')
            self.ax.set_facecolor('#aaddff')
        
        self.ax.set_xlim(120, 145)
        self.ax.set_ylim(25, 45)
        self.ax.set_title("Simulation Viewer")
        
        self.artists = {}
        for team in ["A", "B"]:
            for role in ["commander", "scout", "messenger", "attacker"]:
                style = STYLES[team].copy()
                role_style = STYLES[role]
                
                sc = self.ax.scatter([], [], 
                                     c=style["color"], 
                                     marker=role_style["marker"], 
                                     s=role_style["s"], 
                                     label=f"{team} {role_style['label']}",
                                     zorder=3 if role != "messenger" else 2)
                self.artists[(team, role)] = sc
        
        self.ax.legend(loc='upper right', fontsize='small', markerscale=0.7)
        self.canvas.draw()

    def load_log(self):
        file_path = filedialog.askopenfilename(filetypes=[("Log Files", "*.log *.ndjson *.json"), ("All Files", "*.*")])
        if not file_path: return

        self.root.config(cursor="watch")
        self.lbl_status.config(text="Loading huge log...")
        self.root.update()

        success, msg = self.loader.load_file(file_path)
        
        self.root.config(cursor="")
        
        if success:
            self.lbl_status.config(text=f"Loaded. Range: {self.loader.min_time}-{self.loader.max_time}s")
            self.slider.config(from_=self.loader.min_time, to=self.loader.max_time)
            self.slider.state(["!disabled"])
            self.btn_play.config(state=tk.NORMAL)
            self.var_time.set(self.loader.min_time)
            self.update_plot()
        else:
            messagebox.showerror("Load Error", msg)
            self.lbl_status.config(text="Load Error")

    def update_plot(self):
        if not self.loader.loaded: return
        
        t = self.var_time.get()
        
        h = t // 3600
        m = (t % 3600) // 60
        s = t % 60
        self.lbl_time.config(text=f"Time: {h:02}:{m:02}:{s:02} ({t}s)")

        # ここでDataFrameを生成 (高速化の肝)
        df = self.loader.get_positions_at(t)
        
        if df is None or df.empty:
            for art in self.artists.values():
                art.set_offsets(np.empty((0, 2)))
        else:
            for (team, role), sc in self.artists.items():
                subset = df[(df["team_id"] == team) & (df["role"] == role)]
                if not subset.empty:
                    coords = subset[["lon_deg", "lat_deg"]].to_numpy()
                    sc.set_offsets(coords)
                else:
                    sc.set_offsets(np.empty((0, 2)))

        self.canvas.draw_idle()

    def on_slider_move(self, event):
        if not self.is_playing:
            self.update_plot()

    def toggle_play(self):
        if self.is_playing:
            self.is_playing = False
            self.btn_play.config(text="▶ Play")
            if self.play_job:
                self.root.after_cancel(self.play_job)
                self.play_job = None
        else:
            self.is_playing = True
            self.btn_play.config(text="■ Stop")
            self.play_loop()

    def play_loop(self):
        if not self.is_playing: return

        try:
            step = int(self.var_step.get())
        except:
            step = 10 

        current = self.var_time.get()
        next_t = current + step

        if next_t > self.loader.max_time:
            self.var_time.set(self.loader.max_time)
            self.update_plot()
            self.toggle_play() 
            return

        self.var_time.set(next_t)
        self.update_plot()
        
        self.play_job = self.root.after(100, self.play_loop)

    # --- 地図操作 ---
    def on_scroll(self, event):
        if event.inaxes != self.ax: return
        base_scale = 1.2
        scale_factor = 1/base_scale if event.button == 'up' else base_scale
        
        cur_xlim = self.ax.get_xlim()
        cur_ylim = self.ax.get_ylim()
        xdata, ydata = event.xdata, event.ydata
        
        new_width = (cur_xlim[1] - cur_xlim[0]) * scale_factor
        new_height = (cur_ylim[1] - cur_ylim[0]) * scale_factor
        relx = (cur_xlim[1] - xdata) / (cur_xlim[1] - cur_xlim[0])
        rely = (cur_ylim[1] - ydata) / (cur_ylim[1] - cur_ylim[0])

        self.ax.set_xlim(xdata - new_width * (1-relx), xdata + new_width * relx)
        self.ax.set_ylim(ydata - new_height * (1-rely), ydata + new_height * rely)
        self.canvas.draw_idle()

    def on_press(self, event):
        if event.inaxes != self.ax: return
        if event.button == 1: 
            self.pan_start = (event.xdata, event.ydata)

    def on_motion(self, event):
        if self.pan_start and event.inaxes == self.ax:
            dx = event.xdata - self.pan_start[0]
            dy = event.ydata - self.pan_start[1]
            xlim = self.ax.get_xlim()
            ylim = self.ax.get_ylim()
            self.ax.set_xlim(xlim[0] - dx, xlim[1] - dx)
            self.ax.set_ylim(ylim[0] - dy, ylim[1] - dy)
            self.canvas.draw_idle()

    def on_release(self, event):
        self.pan_start = None

if __name__ == "__main__":
    root = tk.Tk()
    app = ViewerApp(root)
    root.mainloop()