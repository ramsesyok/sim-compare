import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.backends._backend_tk import NavigationToolbar2Tk
import geopandas as gpd
import numpy as np
import json
import random
import math
import sys

# --- 定数・設定 ---
MAP_FILE_PATH = "./map_data/ne_110m_coastline.shp"  # シェープファイルのパス
DEFAULT_FILENAME = "scenario.json"

# WGS84の近似距離計算用 (簡易的に緯度35度付近の係数を使用)
LAT_1DEG_M = 111000
LON_1DEG_M = 91000 

class ScenarioGenerator:
    def __init__(self):
        self.teams = []
        self.performance = {
            "scout": {"comm_range_m": 5000, "detect_range_m": 10000},
            "messenger": {"comm_range_m": 8000},
            "attacker": {"bom_range_m": 1000}
        }
    
    def generate_random_scenario(self, config):
        """設定に基づいてシナリオデータを生成する"""
        self.teams = []
        
        # チーム定義
        team_configs = [
            {
                "id": "A",
                "name": "Alpha Team",
                "color": "blue",
                "cmd_pos": config["A_cmd_pos"],
                "target_pos": config["B_cmd_pos"],
                "num_scouts": config["num_scouts_a"],
                "num_messengers": config["num_messengers_a"],
                "num_attackers": config["num_attackers_a"],
            },
            {
                "id": "B",
                "name": "Bravo Team",
                "color": "red",
                "cmd_pos": config["B_cmd_pos"],
                "target_pos": config["A_cmd_pos"],
                "num_scouts": config["num_scouts_b"],
                "num_messengers": config["num_messengers_b"],
                "num_attackers": config["num_attackers_b"],
            },
        ]

        for tc in team_configs:
            team_obj = {
                "id": tc["id"],
                "name": tc["name"],
                "objects": []
            }

            # 1. 司令官 (Commander)
            cmd = {
                "id": f"{tc['id']}_CMD",
                "role": "commander",
                "start_sec": 0,
                "route": [{
                    "lat_deg": tc["cmd_pos"][0],
                    "lon_deg": tc["cmd_pos"][1],
                    "alt_m": 0.0,
                    "speeds_kph": 0
                }]
            }
            team_obj["objects"].append(cmd)

            # 役割ごとの生成ループ
            roles = [
                ("scout", tc["num_scouts"]),
                ("messenger", tc["num_messengers"]),
                ("attacker", tc["num_attackers"]),
            ]
            
            messenger_ids = [f"{tc['id']}_M{i:02d}" for i in range(tc["num_messengers"])]

            for role, count in roles:
                for i in range(count):
                    obj_id = f"{tc['id']}_{role[0].upper()}{i:02d}"
                    
                    # --- 変更点: 矩形領域での発生位置計算 ---
                    # DX: 東西方向(経度), DY: 南北方向(緯度)
                    # 指定された spawn_dx, spawn_dy を最大距離として、正負の範囲でランダム配置
                    
                    dx_offset_m = random.uniform(-config["spawn_dx_m"], config["spawn_dx_m"])
                    dy_offset_m = random.uniform(-config["spawn_dy_m"], config["spawn_dy_m"])
                    
                    start_lat = tc["cmd_pos"][0] + (dy_offset_m / LAT_1DEG_M)
                    start_lon = tc["cmd_pos"][1] + (dx_offset_m / LON_1DEG_M)
                    
                    # 経路生成 (相手司令官の位置へ向かうランダムウォーク)
                    waypoints = self._generate_random_path(
                        (start_lat, start_lon), 
                        tc["target_pos"], 
                        min_points=5, 
                        noise_scale=0.1, # ノイズの大きさ(度)
                        min_speed=config["min_speed"],
                        max_speed=config["max_speed"]
                    )
                    
                    obj = {
                        "id": obj_id,
                        "role": role,
                        "start_sec": random.randint(0, 3600), # 1時間以内にランダム開始
                        "route": waypoints
                    }

                    # 斥候の場合、伝令リスト(network)を追加
                    if role == "scout":
                        obj["network"] = messenger_ids

                    team_obj["objects"].append(obj)
            
            self.teams.append(team_obj)

    def _generate_random_path(self, start_pos, end_pos, min_points, noise_scale, min_speed, max_speed):
        """始点から終点までのランダムウォーク経路を生成"""
        lat1, lon1 = start_pos
        lat2, lon2 = end_pos
        
        num_points = random.randint(min_points, min_points + 5)
        waypoints = []
        
        # 始点
        waypoints.append({
            "lat_deg": lat1, "lon_deg": lon1, "alt_m": 0.0,
            "speeds_kph": random.uniform(min_speed, max_speed)
        })

        # 中間点 (線形補間 + ノイズ)
        for i in range(1, num_points - 1):
            ratio = i / (num_points - 1)
            base_lat = lat1 + (lat2 - lat1) * ratio
            base_lon = lon1 + (lon2 - lon1) * ratio
            
            # ノイズ付加
            noise_lat = random.uniform(-noise_scale, noise_scale)
            noise_lon = random.uniform(-noise_scale, noise_scale)
            
            waypoints.append({
                "lat_deg": base_lat + noise_lat,
                "lon_deg": base_lon + noise_lon,
                "alt_m": 0.0,
                "speeds_kph": random.uniform(min_speed, max_speed)
            })

        # 終点
        waypoints.append({
            "lat_deg": lat2, "lon_deg": lon2, "alt_m": 0.0,
            "speeds_kph": 0 
        })
        
        return waypoints

    def to_json(self):
        return {
            "performance": self.performance,
            "teams": self.teams
        }

class ScenarioEditorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Simulation Scenario Editor")
        self.root.geometry("1200x800")
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        
        self.generator = ScenarioGenerator()
        self.map_data = None
        
        # ドラッグ操作用
        self.drag_point = None 
        self.pan_start = None
        self.press_event = None

        self._init_gui()
        self._load_map_data()

    def on_closing(self):
        """ウィンドウが閉じられたときにプロセスを強制終了する"""
        self.root.destroy()
        sys.exit(0)

    def _init_gui(self):
        # 左パネル: 設定
        control_frame = ttk.Frame(self.root, padding="10")
        control_frame.pack(side=tk.LEFT, fill=tk.Y)
        
        ttk.Label(control_frame, text="設定パラメータ", font=("", 12, "bold")).pack(pady=5)
        
        self.inputs = {}
        def add_entry(label, key, default):
            frame = ttk.Frame(control_frame)
            frame.pack(fill=tk.X, pady=2)
            ttk.Label(frame, text=label).pack(side=tk.LEFT)
            entry = ttk.Entry(frame, width=10)
            entry.pack(side=tk.RIGHT)
            entry.insert(0, str(default))
            self.inputs[key] = entry

        add_entry("Cmd A Lat:", "A_lat", 33.593285592006765) # 福岡タワー
        add_entry("Cmd A Lon:", "A_lon", 130.35150899543166)
        add_entry("Cmd B Lat:", "B_lat", 31.23971183320547) # 上海タワー
        add_entry("Cmd B Lon:", "B_lon", 121.49974993544004)
        
        ttk.Separator(control_frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=10)
        
        add_entry("Scouts (A):", "n_scout_a", 10)
        add_entry("Messengers (A):", "n_msg_a", 20)
        add_entry("Attackers (A):", "n_atk_a", 50)
        add_entry("Scouts (B):", "n_scout_b", 10)
        add_entry("Messengers (B):", "n_msg_b", 20)
        add_entry("Attackers (B):", "n_atk_b", 50)
        
        ttk.Separator(control_frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=10)

        # --- 変更点: DX, DYの入力欄に変更 ---
        add_entry("Spawn DX (m):", "spawn_dx", 10000) # 東西の広がり
        add_entry("Spawn DY (m):", "spawn_dy", 30000) # 南北の広がり
        
        add_entry("Min Speed (km/h):", "min_spd", 20)
        add_entry("Max Speed (km/h):", "max_spd", 80)

        ttk.Separator(control_frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=10)
        
        ttk.Button(control_frame, text="Generate / Reset", command=self.generate_scenario).pack(fill=tk.X, pady=5)
        ttk.Button(control_frame, text="Save JSON", command=self.save_json).pack(fill=tk.X, pady=5)

        help_text = (
            "[操作方法]\n"
            "・ホイール: ズーム\n"
            "・背景ドラッグ: 地図移動\n"
            "・□点ドラッグ: 伝令経路修正"
        )
        ttk.Label(control_frame, text=help_text, foreground="gray").pack(pady=20)

        # 右パネル: 地図プロット
        plot_frame = ttk.Frame(self.root)
        plot_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        self.fig, self.ax = plt.subplots(figsize=(8, 8))
        self.canvas = FigureCanvasTkAgg(self.fig, master=plot_frame)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        toolbar = NavigationToolbar2Tk(self.canvas, plot_frame)
        toolbar.update()
        
        # イベント接続
        self.canvas.mpl_connect("button_press_event", self.on_press)
        self.canvas.mpl_connect("motion_notify_event", self.on_motion)
        self.canvas.mpl_connect("button_release_event", self.on_release)
        self.canvas.mpl_connect("scroll_event", self.on_scroll)

    def _load_map_data(self):
        try:
            self.map_data = gpd.read_file(MAP_FILE_PATH)
        except Exception as e:
            messagebox.showwarning("Map Load Error", f"地図読込失敗: {e}")
            self.map_data = None

    def get_config(self):
        try:
            return {
                "A_cmd_pos": (float(self.inputs["A_lat"].get()), float(self.inputs["A_lon"].get())),
                "B_cmd_pos": (float(self.inputs["B_lat"].get()), float(self.inputs["B_lon"].get())),
                "num_scouts_a": int(self.inputs["n_scout_a"].get()),
                "num_scouts_b": int(self.inputs["n_scout_b"].get()),
                "num_messengers_a": int(self.inputs["n_msg_a"].get()),
                "num_messengers_b": int(self.inputs["n_msg_b"].get()),
                "num_attackers_a": int(self.inputs["n_atk_a"].get()),
                "num_attackers_b": int(self.inputs["n_atk_b"].get()),
                # --- 変更点: DX, DYを取得 ---
                "spawn_dx_m": float(self.inputs["spawn_dx"].get()),
                "spawn_dy_m": float(self.inputs["spawn_dy"].get()),
                "min_speed": float(self.inputs["min_spd"].get()),
                "max_speed": float(self.inputs["max_spd"].get()),
            }
        except ValueError:
            messagebox.showerror("Input Error", "数値入力が不正です。")
            return None

    def generate_scenario(self):
        config = self.get_config()
        if not config: return
        self.generator.generate_random_scenario(config)
        self.redraw()

    def redraw(self):
        cur_xlim = self.ax.get_xlim()
        cur_ylim = self.ax.get_ylim()
        is_default_view = (cur_xlim == (0.0, 1.0) and cur_ylim == (0.0, 1.0))

        self.ax.clear()
        
        if self.map_data is not None:
            self.map_data.plot(ax=self.ax, color='lightgray', edgecolor='black')
        else:
            self.ax.grid(True)

        if is_default_view:
            self.ax.set_xlim(125, 145)
            self.ax.set_ylim(30, 45)
        else:
            self.ax.set_xlim(cur_xlim)
            self.ax.set_ylim(cur_ylim)

        self.ax.set_title("Scenario Preview")

        self.plot_refs = {} 
        colors = {"A": "blue", "B": "red"}
        
        for t_idx, team in enumerate(self.generator.teams):
            c = colors.get(team["id"], "green")
            for o_idx, obj in enumerate(team["objects"]):
                lats = [wp["lat_deg"] for wp in obj["route"]]
                lons = [wp["lon_deg"] for wp in obj["route"]]
                
                style = "-"
                if obj["role"] == "messenger": style = "--"
                elif obj["role"] == "scout": style = ":"
                
                self.ax.plot(lons, lats, color=c, linestyle=style, alpha=0.6, linewidth=1)
                
                marker = 'o'
                if obj["role"] == "commander": marker = '*'
                elif obj["role"] == "attacker": marker = 'x'
                elif obj["role"] == "messenger": marker = 's' 
                elif obj["role"] == "scout": marker = '^'

                if obj["role"] == "messenger":
                    for w_idx, (la, lo) in enumerate(zip(lats, lons)):
                        sc = self.ax.scatter(lo, la, c=c, marker=marker, s=30, zorder=10, picker=5)
                        self.plot_refs[sc] = (t_idx, o_idx, w_idx)
                else:
                    self.ax.scatter(lons, lats, c=c, marker=marker, s=20, zorder=3)

        self.canvas.draw()

    def on_scroll(self, event):
        if event.inaxes != self.ax: return

        base_scale = 1.2
        if event.button == 'up':
            scale_factor = 1 / base_scale
        elif event.button == 'down':
            scale_factor = base_scale
        else:
            return

        cur_xlim = self.ax.get_xlim()
        cur_ylim = self.ax.get_ylim()
        xdata = event.xdata
        ydata = event.ydata

        new_width = (cur_xlim[1] - cur_xlim[0]) * scale_factor
        new_height = (cur_ylim[1] - cur_ylim[0]) * scale_factor

        relx = (cur_xlim[1] - xdata) / (cur_xlim[1] - cur_xlim[0])
        rely = (cur_ylim[1] - ydata) / (cur_ylim[1] - cur_ylim[0])

        # --- 修正済み: 引数を2つに分離 ---
        self.ax.set_xlim(xdata - new_width * (1-relx), xdata + new_width * relx)
        self.ax.set_ylim(ydata - new_height * (1-rely), ydata + new_height * rely)
        
        self.canvas.draw_idle()

    def on_press(self, event):
        if event.inaxes != self.ax: return
        
        click_point = (event.xdata, event.ydata)
        min_dist = float('inf')
        target = None
        
        for art, val in self.plot_refs.items():
            pos = art.get_offsets()[0]
            dist = (pos[0] - click_point[0])**2 + (pos[1] - click_point[1])**2
            if dist < min_dist:
                min_dist = dist
                target = (val, art)

        curr_span = self.ax.get_xlim()[1] - self.ax.get_xlim()[0]
        threshold = (curr_span * 0.02) ** 2 

        if target and min_dist < threshold:
            (t_idx, o_idx, w_idx), art = target
            self.drag_point = (t_idx, o_idx, w_idx, art)
        else:
            self.pan_start = (event.xdata, event.ydata)

        self.press_event = event

    def on_motion(self, event):
        if event.inaxes != self.ax: return

        if self.drag_point:
            t_idx, o_idx, w_idx, art = self.drag_point
            self.generator.teams[t_idx]["objects"][o_idx]["route"][w_idx]["lat_deg"] = event.ydata
            self.generator.teams[t_idx]["objects"][o_idx]["route"][w_idx]["lon_deg"] = event.xdata
            art.set_offsets([event.xdata, event.ydata])
            self.canvas.draw_idle()

        elif self.pan_start:
            dx = event.xdata - self.pan_start[0]
            dy = event.ydata - self.pan_start[1]
            xlim = self.ax.get_xlim()
            ylim = self.ax.get_ylim()
            self.ax.set_xlim(xlim[0] - dx, xlim[1] - dx)
            self.ax.set_ylim(ylim[0] - dy, ylim[1] - dy)
            self.canvas.draw_idle()

    def on_release(self, event):
        self.pan_start = None
        if self.drag_point:
            self.drag_point = None
            self.redraw()

    def save_json(self):
        data = self.generator.to_json()
        file_path = filedialog.asksaveasfilename(defaultextension=".json", initialfile=DEFAULT_FILENAME)
        if file_path:
            with open(file_path, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=2)
            messagebox.showinfo("Saved", f"File saved to {file_path}")

if __name__ == "__main__":
    root = tk.Tk()
    app = ScenarioEditorApp(root)
    root.mainloop()
