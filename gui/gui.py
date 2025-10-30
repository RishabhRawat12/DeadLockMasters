import tkinter as tk
from tkinter import ttk, simpledialog, messagebox, scrolledtext
from PIL import Image, ImageTk, ImageDraw, ImageFont
import subprocess
import threading
import json
import time
import os

# --- Main GUI Application ---
class DeadlockApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Deadlock Master Visualizer")
        self.geometry("1400x900")
        
        # Store the C++ engine process
        self.cpp_process = None
        
        # Data for rendering
        self.proc_coords = {} # 'P0': (x, y)
        self.res_coords = {}  # 'R0': (x, y)
        self.state_data = {}  # Last JSON state
        self.is_deadlocked = False
        self.deadlock_cycle = []
        
        # --- Create Layout ---
        # Main container
        main_frame = ttk.Frame(self)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        # Left Column (Controls)
        left_frame = ttk.Frame(main_frame, width=300)
        left_frame.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 10))

        # Center Column (Visual Graph)
        center_frame = ttk.Frame(main_frame)
        center_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        # Bottom Row (Log)
        log_frame = ttk.Frame(self, height=200)
        log_frame.pack(side=tk.BOTTOM, fill=tk.X, padx=10, pady=(10, 0))

        # --- Populate Controls (Left Frame) ---
        self.build_controls(left_frame)

        # --- Populate Graph (Center Frame) ---
        self.canvas = tk.Canvas(center_frame, bg="#2B2B2B")
        self.canvas.pack(fill=tk.BOTH, expand=True)
        
        # Bind mouse events for dragging
        self.canvas.tag_bind("process", "<Button-1>", self.on_drag_start)
        self.canvas.tag_bind("process", "<B1-Motion>", self.on_drag_motion)
        self.canvas.tag_bind("resource", "<Button-1>", self.on_drag_start)
        self.canvas.tag_bind("resource", "<B1-Motion>", self.on_drag_motion)
        self._drag_data = {"x": 0, "y": 0, "item": None}

        # --- Populate Log (Bottom Frame) ---
        self.log_text = scrolledtext.ScrolledText(log_frame, height=10, bg="#1E1E1E", fg="#D4D4D4", font=("Consolas", 10), wrap=tk.WORD)
        self.log_text.pack(fill=tk.BOTH, expand=True)

        # --- Start Engine ---
        self.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.start_cpp_engine()

    def build_controls(self, parent):
        # --- Strategy ---
        f_strat = ttk.LabelFrame(parent, text="1. Strategy")
        f_strat.pack(fill=tk.X, pady=5)
        self.strategy_var = tk.StringVar(value="DETECT")
        ttk.Radiobutton(f_strat, text="Detect & Recovery", variable=self.strategy_var, value="DETECT", command=self.set_strategy).pack(anchor=tk.W)
        ttk.Radiobutton(f_strat, text="Avoidance (Banker's)", variable=self.strategy_var, value="AVOID", command=self.set_strategy).pack(anchor=tk.W)

        # --- Add Components ---
        f_add = ttk.LabelFrame(parent, text="2. Add Components")
        f_add.pack(fill=tk.X, pady=5)
        # Process
        ttk.Label(f_add, text="Process ID:").grid(row=0, column=0, sticky=tk.W, padx=5, pady=2)
        self.add_p_id = ttk.Entry(f_add, width=5)
        self.add_p_id.grid(row=0, column=1, sticky=tk.W, padx=5, pady=2)
        ttk.Button(f_add, text="Add Process", command=self.add_process).grid(row=0, column=2, sticky=tk.EW, padx=5, pady=2)
        # Resource
        ttk.Label(f_add, text="Resource ID:").grid(row=1, column=0, sticky=tk.W, padx=5, pady=2)
        self.add_r_id = ttk.Entry(f_add, width=5)
        self.add_r_id.grid(row=1, column=1, sticky=tk.W, padx=5, pady=2)
        ttk.Label(f_add, text="Instances:").grid(row=2, column=0, sticky=tk.W, padx=5, pady=2)
        self.add_r_count = ttk.Entry(f_add, width=5)
        self.add_r_count.grid(row=2, column=1, sticky=tk.W, padx=5, pady=2)
        ttk.Button(f_add, text="Add Resource", command=self.add_resource).grid(row=1, column=2, rowspan=2, sticky=tk.EW, padx=5, pady=2)

        # --- Declare Max Need ---
        f_max = ttk.LabelFrame(parent, text="3. Declare Max Need (Banker's)")
        f_max.pack(fill=tk.X, pady=5)
        ttk.Label(f_max, text="Process:").grid(row=0, column=0, sticky=tk.W, padx=5, pady=2)
        self.max_p_var = tk.StringVar()
        self.max_p_combo = ttk.Combobox(f_max, textvariable=self.max_p_var, width=7)
        self.max_p_combo.grid(row=0, column=1, padx=5, pady=2)
        ttk.Label(f_max, text="Resource:").grid(row=1, column=0, sticky=tk.W, padx=5, pady=2)
        self.max_r_var = tk.StringVar()
        self.max_r_combo = ttk.Combobox(f_max, textvariable=self.max_r_var, width=7)
        self.max_r_combo.grid(row=1, column=1, padx=5, pady=2)
        ttk.Label(f_max, text="Count:").grid(row=2, column=0, sticky=tk.W, padx=5, pady=2)
        self.max_count_entry = ttk.Entry(f_max, width=5)
        self.max_count_entry.grid(row=2, column=1, padx=5, pady=2)
        ttk.Button(f_max, text="Declare", command=self.declare_max).grid(row=0, column=2, rowspan=3, sticky=tk.EW, padx=5, pady=2)

        # --- Execute Event ---
        f_event = ttk.LabelFrame(parent, text="4. Execute Event")
        f_event.pack(fill=tk.X, pady=5)
        ttk.Label(f_event, text="Process:").grid(row=0, column=0, sticky=tk.W, padx=5, pady=2)
        self.event_p_var = tk.StringVar()
        self.event_p_combo = ttk.Combobox(f_event, textvariable=self.event_p_var, width=7)
        self.event_p_combo.grid(row=0, column=1, padx=5, pady=2)
        ttk.Label(f_event, text="Action:").grid(row=1, column=0, sticky=tk.W, padx=5, pady=2)
        self.event_action_var = tk.StringVar(value="REQUEST")
        self.event_action_combo = ttk.Combobox(f_event, textvariable=self.event_action_var, width=7, values=["REQUEST", "RELEASE"])
        self.event_action_combo.grid(row=1, column=1, padx=5, pady=2)
        ttk.Label(f_event, text="Resource:").grid(row=2, column=0, sticky=tk.W, padx=5, pady=2)
        self.event_r_var = tk.StringVar()
        self.event_r_combo = ttk.Combobox(f_event, textvariable=self.event_r_var, width=7)
        self.event_r_combo.grid(row=2, column=1, padx=5, pady=2)
        ttk.Label(f_event, text="Count:").grid(row=3, column=0, sticky=tk.W, padx=5, pady=2)
        self.event_count_entry = ttk.Entry(f_event, width=5)
        self.event_count_entry.grid(row=3, column=1, padx=5, pady=2)
        ttk.Button(f_event, text="Run Event", command=self.run_event).grid(row=0, column=2, rowspan=4, sticky=tk.NSEW, padx=5, pady=2)

        # --- Recovery ---
        f_rec = ttk.LabelFrame(parent, text="5. Recovery (Detect Mode)")
        f_rec.pack(fill=tk.X, pady=5)
        self.recover_button = ttk.Button(f_rec, text="Resolve Deadlock", command=self.run_recovery, state=tk.DISABLED)
        self.recover_button.pack(fill=tk.X, padx=5, pady=5)

    # --- GUI -> C++ ---

    def set_strategy(self):
        self.send_command(f"S {self.strategy_var.get()}")

    def add_process(self):
        pid = self.add_p_id.get()
        if pid:
            self.send_command(f"P {pid}")
            self.add_p_id.delete(0, tk.END)

    def add_resource(self):
        rid = self.add_r_id.get()
        count = self.add_r_count.get()
        if rid and count:
            self.send_command(f"R {rid} {count}")
            self.add_r_id.delete(0, tk.END)
            self.add_r_count.delete(0, tk.END)

    def declare_max(self):
        pid = self.max_p_var.get()
        rid = self.max_r_var.get()
        count = self.max_count_entry.get()
        if pid and rid and count:
            self.send_command(f"M {pid} {rid} {count}")

    def run_event(self):
        pid = self.event_p_var.get()
        action = self.event_action_var.get()
        rid = self.event_r_var.get()
        count = self.event_count_entry.get()
        if pid and action and rid and count:
            self.send_command(f"E {pid} {action} {rid} {count}")

    def run_recovery(self):
        self.send_command("C") # 'C' for reCovery

    # --- C++ Engine Communication ---

    def start_cpp_engine(self):
        # Path to the C++ executable in the 'bin' folder
        # Assumes gui.py is in 'gui/' and DeadlockMaster.exe is in 'bin/'
        script_dir = os.path.dirname(__file__)
        exe_path = os.path.join(script_dir, "../bin/DeadlockMaster.exe")
        
        if not os.path.exists(exe_path):
            messagebox.showerror("Error", f"Could not find engine at {exe_path}\nPlease compile the C++ project first.")
            self.destroy()
            return
            
        self.cpp_process = subprocess.Popen(
            [exe_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
            creationflags=subprocess.CREATE_NO_WINDOW # Windows only: hide console
        )
        
        self.stdout_thread = threading.Thread(target=self.read_stdout, daemon=True)
        self.stderr_thread = threading.Thread(target=self.read_stderr, daemon=True)
        self.stdout_thread.start()
        self.stderr_thread.start()
        
        # Set initial strategy
        self.set_strategy()
        # Request initial state
        self.send_command("X")

    def send_command(self, command):
        if self.cpp_process and self.cpp_process.poll() is None:
            try:
                self.cpp_process.stdin.write(command + "\n")
                self.cpp_process.stdin.flush()
            except Exception as e:
                self.log_message(f"ERROR: Failed to send command: {e}")
        elif not self.cpp_process:
            self.log_message("ERROR: C++ Engine not running.")

    def log_message(self, message):
        def _log():
            self.log_text.config(state=tk.NORMAL)
            self.log_text.insert(tk.END, message + "\n")
            self.log_text.see(tk.END)
            self.log_text.config(state=tk.DISABLED)
        # Schedule GUI updates on the main thread
        if hasattr(self, 'log_text'):
            self.after(0, _log)

    def read_stdout(self):
        current_state_json = ""
        is_reading_state = False
        while True:
            try:
                line = self.cpp_process.stdout.readline()
                if not line: break
                
                line = line.strip()
                if line == "---STATE_BEGIN---":
                    current_state_json = ""
                    is_reading_state = True
                elif line == "---STATE_END---":
                    is_reading_state = False
                    try:
                        self.state_data = json.loads(current_state_json)
                        # Schedule GUI update on main thread
                        self.after(0, self.update_gui_from_state)
                    except json.JSONDecodeError as e:
                        print(f"JSON Parse Error: {e}\nData: {current_state_json}")
                elif is_reading_state:
                    current_state_json += line
                
            except Exception as e:
                print(f"Stdout read error: {e}")
                break

    def read_stderr(self):
        while True:
            try:
                line = self.cpp_process.stderr.readline()
                if not line: break
                self.log_message(f"CPP_ERROR: {line.strip()}")
            except:
                break

    # --- GUI Update Logic ---

    def update_gui_from_state(self):
        if not self.state_data: return
        
        # 1. Update Logs
        for msg in self.state_data.get('log', []):
            self.log_message(f"[Engine] {msg}")

        # 2. Update Comboboxes
        proc_ids = [p['id'] for p in self.state_data.get('processes', [])]
        res_ids = [r['id'] for r in self.state_data.get('resources', [])]
        self.max_p_combo['values'] = proc_ids
        self.event_p_combo['values'] = proc_ids
        self.max_r_combo['values'] = res_ids
        self.event_r_combo['values'] = res_ids
        
        # 3. Update Deadlock Status
        self.deadlock_cycle = self.state_data.get('deadlock_cycle', [])
        self.is_deadlocked = bool(self.deadlock_cycle)
        
        if self.is_deadlocked and self.strategy_var.get() == "DETECT":
            self.recover_button.config(state=tk.NORMAL)
        else:
            self.recover_button.config(state=tk.DISABLED)

        # 4. Redraw Graph
        self.draw_graph()

    def draw_graph(self):
        self.canvas.delete("all")
        if not self.state_data: return

        # Auto-layout logic (simple)
        self.update_node_coords(self.state_data.get('processes', []), self.state_data.get('resources', []))

        # Draw Resources (Squares)
        for r in self.state_data.get('resources', []):
            rid = r['id']
            x, y = self.res_coords[rid]
            tags = ("resource", f"res_{rid}")
            self.canvas.create_rectangle(x-20, y-20, x+20, y+20, fill="#3C3F41", outline="#888888", width=2, tags=tags)
            self.canvas.create_text(x, y, text=f"R{rid}\n({r['available']}/{r['total']})", fill="#BBBBBB", font=("Arial", 9, "bold"), tags=tags)

        # Draw Processes (Circles)
        for p in self.state_data.get('processes', []):
            pid = p['id']
            x, y = self.proc_coords[pid]
            
            # Determine color
            fill_color = "#3C7A3C" # Green (Running)
            if any(w['process_id'] == pid for w in self.state_data.get('waiting', [])):
                fill_color = "#A9892D" # Yellow (Waiting)
            if pid in self.deadlock_cycle:
                fill_color = "#A9302D" # Red (Deadlocked)

            tags = ("process", f"proc_{pid}")
            self.canvas.create_oval(x-20, y-20, x+20, y+20, fill=fill_color, outline="#888888", width=2, tags=tags)
            self.canvas.create_text(x, y, text=f"P{pid}\nPrio: {p['priority']}", fill="#DDDDDD", font=("Arial", 9, "bold"), tags=tags)
        
        # Draw Arrows
        # Allocation (Resource -> Process)
        for p in self.state_data.get('processes', []):
            pid = p['id']
            for held in p.get('held', []):
                rid = held['id']
                if pid in self.proc_coords and rid in self.res_coords:
                    self.draw_arrow(self.res_coords[rid], self.proc_coords[pid], f"{held['count']}", "#6A8759")

        # Waiting (Process -> Resource)
        for w in self.state_data.get('waiting', []):
            pid = w['process_id']
            rid = w['resource_id']
            color = "#A9892D" # Yellow
            if pid in self.deadlock_cycle:
                color = "#A9302D" # Red
                
            if pid in self.proc_coords and rid in self.res_coords:
                self.draw_arrow(self.proc_coords[pid], self.res_coords[rid], f"{w['count']}", color)

    def draw_arrow(self, p1, p2, text, color):
        x1, y1 = p1
        x2, y2 = p2
        self.canvas.create_line(x1, y1, x2, y2, arrow=tk.LAST, fill=color, width=2)
        # Draw count label near the middle of the line
        self.canvas.create_text((x1+x2)/2, (y1+y2)/2 - 10, text=text, fill=color, font=("Arial", 9, "bold"))
        
    def update_node_coords(self, processes, resources):
        # Simple layout: Processes on left, Resources on right
        canvas_w = self.canvas.winfo_width()
        canvas_h = self.canvas.winfo_height()
        
        proc_x = canvas_w * 0.25
        for i, p in enumerate(processes):
            pid = p['id']
            if pid not in self.proc_coords:
                y = (canvas_h / (len(processes) + 1)) * (i + 1)
                self.proc_coords[pid] = (proc_x, y)
                
        res_x = canvas_w * 0.75
        for i, r in enumerate(resources):
            rid = r['id']
            if rid not in self.res_coords:
                y = (canvas_h / (len(resources) + 1)) * (i + 1)
                self.res_coords[rid] = (res_x, y)

    # --- Drag and Drop ---
    def on_drag_start(self, event):
        self._drag_data["item"] = self.canvas.find_closest(event.x, event.y)[0]
        self._drag_data["x"] = event.x
        self._drag_data["y"] = event.y

    def on_drag_motion(self, event):
        dx = event.x - self._drag_data["x"]
        dy = event.y - self._drag_data["y"]
        self.canvas.move(self._drag_data["item"], dx, dy)
        self._drag_data["x"] = event.x
        self._drag_data["y"] = event.y
        
        # Update internal coords
        tags = self.canvas.gettags(self._drag_data["item"])
        if "process" in tags:
            pid = [t.split("_")[1] for t in tags if t.startswith("proc_")][0]
            self.proc_coords[int(pid)] = (event.x, event.y)
        elif "resource" in tags:
            rid = [t.split("_")[1] for t in tags if t.startswith("res_")][0]
            self.res_coords[int(rid)] = (event.x, event.y)
        
        self.draw_graph() # Redraw all arrows while dragging

    def on_closing(self):
        # Clean up C++ child process
        if self.cpp_process:
            self.cpp_process.terminate()
            self.cpp_process.wait()
        self.destroy()

# --- Run the App ---
if __name__ == "__main__":
    app = DeadlockApp()
    app.mainloop()