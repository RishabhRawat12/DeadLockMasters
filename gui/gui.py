import tkinter as tk
from tkinter import ttk, simpledialog, messagebox, scrolledtext
from PIL import Image, ImageTk, ImageDraw, ImageFont
import subprocess
import threading
import json
import time
import os
import re

# --- Color Constants for Bright Theme ---
BRIGHT_BG = "#F0F0F0"       # Light Gray/White Background
BRIGHT_CANVAS_BG = "#FFFFFF" # Pure White Canvas
TEXT_COLOR = "#333333"      # Dark Text Color

# Node Colors (More Vibrant)
NODE_GREEN = "#5CB85C"      # Vibrant Green (Running)
NODE_YELLOW = "#FFC107"     # Vibrant Yellow (Waiting)
NODE_RED = "#DC3545"        # Vibrant Red (Deadlocked)
NODE_OUTLINE = "#555555"    # Darker outline for contrast

# Arrow Colors
ARROW_ALLOCATION = "#5CB85C" # Green for holding arrows
ARROW_WAITING = "#FFC107"    # Yellow for waiting arrows

# --- Main GUI Application ---
class DeadlockApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Deadlock Master Visualizer")
        self.geometry("1400x900")
        
        # Store the C++ engine process
        self.cpp_process = None
        
        # Data for rendering
        self.proc_coords = {} 
        self.res_coords = {} 
        self.state_data = {}
        self.is_deadlocked = False
        self.deadlock_cycle = []
        self.flashing = False # New state for controlling the deadlock animation
        
        # --- Create Layout ---
        main_frame = ttk.Frame(self)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        # Left Column (Controls) - Use Light BG
        left_frame = ttk.Frame(main_frame, width=300, style='Bright.TFrame')
        left_frame.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 10))

        # Center Column (Visual Graph)
        center_frame = ttk.Frame(main_frame)
        center_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        # Bottom Row (Log) - Use Light BG. 
        log_frame = ttk.Frame(self, height=200, style='Bright.TFrame')
        log_frame.pack(side=tk.BOTTOM, fill=tk.X, padx=10, pady=(10, 0))

        # Apply custom style to make the application look brighter
        style = ttk.Style(self)
        style.theme_use('clam')
        style.configure('Bright.TFrame', background=BRIGHT_BG)
        style.configure('TLabel', background=BRIGHT_BG, foreground=TEXT_COLOR)
        style.configure('TRadiobutton', background=BRIGHT_BG, foreground=TEXT_COLOR)
        style.configure('TButton', background='#DDDDDD', foreground=TEXT_COLOR)
        style.configure('TEntry', fieldbackground='#FFFFFF', foreground=TEXT_COLOR)

        # --- Populate Controls (Left Frame) ---
        self.build_controls(left_frame)

        # --- Populate Graph (Center Frame) ---
        self.canvas = tk.Canvas(center_frame, bg=BRIGHT_CANVAS_BG)
        self.canvas.pack(fill=tk.BOTH, expand=True)
        
        self.canvas.tag_bind("process", "<Button-1>", self.on_drag_start)
        self.canvas.tag_bind("process", "<B1-Motion>", self.on_drag_motion)
        self.canvas.tag_bind("resource", "<Button-1>", self.on_drag_start)
        self.canvas.tag_bind("resource", "<B1-Motion>", self.on_drag_motion)
        self._drag_data = {"x": 0, "y": 0, "item": None}

        # --- Populate Log (Bottom Frame) ---
        self.log_text = scrolledtext.ScrolledText(log_frame, height=20, bg=BRIGHT_BG, fg=TEXT_COLOR, font=("Consolas", 10), wrap=tk.WORD)
        self.log_text.pack(fill=tk.BOTH, expand=True)
        
        self.log_text.tag_config("error", foreground=NODE_RED)
        self.log_text.tag_config("success", foreground=NODE_GREEN)
        self.log_text.tag_config("warning", foreground="#FFA500")
        self.log_text.tag_config("info", foreground=TEXT_COLOR)

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

    # --- GUI -> C++ (Same as original) ---

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
        self.send_command("C")

    # --- C++ Engine Communication (Minor Log Change) ---

    def start_cpp_engine(self):
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
            creationflags=subprocess.CREATE_NO_WINDOW
        )
        
        self.stdout_thread = threading.Thread(target=self.read_stdout, daemon=True)
        self.stderr_thread = threading.Thread(target=self.read_stderr, daemon=True)
        self.stdout_thread.start()
        self.stderr_thread.start()
        
        self.set_strategy()
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
            
            tag = "info" 
            if "DEADLOCK DETECTED" in message or "Request DENIED" in message or "CRITICAL:" in message or "Error:" in message:
                tag = "error"
            elif "Request GRANTED" in message or "Recovery successful" in message or "releases" in message:
                tag = "success"
            elif "Aging: Increased P" in message:
                tag = "warning"
            
            self.log_text.insert(tk.END, message + "\n", tag)
            self.log_text.see(tk.END)
            self.log_text.config(state=tk.DISABLED)

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

    # --- Animation Logic (Re-used/Modified from previous step) ---

    def animate_request(self, pId, rId, count, action):
        """Initializes and starts the animation of a resource token."""
        
        start_x, start_y = self.proc_coords.get(pId, (0, 0))
        end_x, end_y = self.res_coords.get(rId, (0, 0))
        
        if start_x == 0 and start_y == 0:
            self.draw_graph()
            return

        if action == 'releases':
            token_color = ARROW_ALLOCATION
        else:
            token_color = ARROW_WAITING

        token_id = self.canvas.create_oval(
            start_x - 5, start_y - 5, start_x + 5, start_y + 5, 
            fill=token_color, outline=token_color, tags=("moving_token")
        )
        
        text_id = self.canvas.create_text(
            start_x, start_y - 10, text=str(count), fill=TEXT_COLOR, 
            font=("Arial", 7, "bold"), tags=("moving_token_text")
        )

        total_distance = ((end_x - start_x)**2 + (end_y - start_y)**2)**0.5
        steps = max(20, int(total_distance / 10)) 
        delay = 15

        self.move_token(token_id, text_id, start_x, start_y, end_x, end_y, steps, delay, 0, action)


    def move_token(self, token_id, text_id, start_x, start_y, end_x, end_y, steps, delay, step_count, action):
        """Recursively moves the token across the canvas."""

        if not self.canvas.winfo_exists() or step_count >= steps:
            self.canvas.delete(token_id)
            self.canvas.delete(text_id)
            self.draw_graph() 
            
            # After a potential deadlock/aging trigger, restart the animation if needed
            if self.is_deadlocked and not self.flashing and self.strategy_var.get() == "DETECT":
                self.flashing = True
                self.flash_deadlock_cycle()
            
            return

        progress = step_count / steps
        current_x = start_x + (end_x - start_x) * progress
        current_y = start_y + (end_y - start_y) * progress

        self.canvas.coords(
            token_id, 
            current_x - 5, current_y - 5, 
            current_x + 5, current_y + 5
        )
        self.canvas.coords(text_id, current_x, current_y)

        step_count += 1
        self.after(delay, lambda: self.move_token(
            token_id, text_id, start_x, start_y, end_x, end_y, steps, delay, step_count, action
        ))

    
    def flash_deadlock_cycle(self, state=0):
        """Creates the pulsing/flashing animation for deadlocked processes."""
        if not self.is_deadlocked or not self.flashing:
            # Stop the flashing and revert to static draw
            self.draw_graph(flash_color=NODE_RED)
            return

        flash_color = NODE_RED if state % 2 == 0 else BRIGHT_CANVAS_BG # Flash between Red and White
        
        # Redraw the graph for one flash state
        self.draw_graph(flash_color=flash_color)
        
        # Continue the loop
        self.after(300, lambda: self.flash_deadlock_cycle(state + 1))


    def pulse_aging_process(self, pId, start_time):
        """Creates a temporary vibrant pulse for an aging process."""
        duration = 500 # 500ms pulse
        if (time.time() * 1000) - start_time > duration:
            self.draw_graph()
            return
            
        pulse_color = "#FFD700" # Bright Gold
        p = self.get_process_state(pId)
        
        if p:
            x, y = self.proc_coords.get(pId, (0,0))
            
            # Draw a temporary glowing circle *under* the actual node
            r = 25 + 10 * ((time.time() * 1000 - start_time) / duration) # Radius shrinks/expands
            
            self.canvas.create_oval(x-r, y-r, x+r, y+r, 
                                    outline=pulse_color, width=4, 
                                    tags=("pulse"))
            
            # Continue the pulse animation
            self.after(50, lambda: self.pulse_aging_process(pId, start_time))
        
        # Redraw the graph to cover the pulse element (it will draw the main graph, then call the pulse)
        self.draw_graph() 
        
    def animate_victim_mark(self, victimId, phase=0):
        """Draws a pulsing 'X' on the victim process."""
        if not self.canvas.winfo_exists(): return
        
        if phase >= 10: # Stop after 10 phases (1.5 seconds)
            self.draw_graph()
            return
            
        x, y = self.proc_coords.get(victimId, (0, 0))
        
        self.draw_graph() 
        
        color = NODE_RED if phase % 2 == 0 else BRIGHT_CANVAS_BG
        width = 4 if phase % 2 == 0 else 2
        
        self.canvas.create_line(x - 15, y - 15, x + 15, y + 15, fill=color, width=width, tags=("victim_mark"))
        self.canvas.create_line(x - 15, y + 15, x + 15, y - 15, fill=color, width=width, tags=("victim_mark"))

        self.after(150, lambda: self.animate_victim_mark(victimId, phase + 1))


    def get_process_state(self, pId):
        """Helper to get process data."""
        for p in self.state_data.get('processes', []):
            if p['id'] == pId:
                return p
        return None
    
    # --- GUI Update Logic (Triggers animations) ---

    def update_gui_from_state(self):
        if not self.state_data: return
        
        logs = self.state_data.get('log', [])
        
        is_event_log = False
        is_aging_log = False
        
        if logs:
            first_log = logs[0]
            
            # 1. Check for Request/Release (triggers token animation)
            match_event = re.search(r"P(\d+)\s+(requests|releases)\s+(\d+)\s+of\s+R(\d+)", first_log)
            if match_event:
                is_event_log = True
                pId = int(match_event.group(1))
                action = match_event.group(2)
                count = int(match_event.group(3))
                rId = int(match_event.group(4))
                self.animate_request(pId, rId, count, action) 

            # 2. Check for Aging Priority Boost (triggers pulse animation)
            match_aging = re.search(r"\*\*\* Aging: Increased P(\d+) priority to \d+ \*\*\*", first_log)
            if match_aging:
                pId = int(match_aging.group(1))
                self.pulse_aging_process(pId, time.time() * 1000)
                is_aging_log = True

            # 3. Check for Victim Selection (triggers mark animation)
            match_victim = re.search(r"Selected P(\d+) as victim", first_log)
            if match_victim:
                victimId = int(match_victim.group(1))
                
                self.flashing = False 
                self.animate_victim_mark(victimId)
                is_event_log = True 

            # 4. Check for Recovery Success/Failure (stops any residual animation/flashing)
            if re.search(r"Recovery successful|Recovery FAILED", first_log):
                 self.flashing = False

        # 5. Update Logs
        for msg in logs:
            self.log_message(f"[Engine] {msg}")

        # 6. Update Controls/Deadlock Status (Standard logic)
        proc_ids = [p['id'] for p in self.state_data.get('processes', [])]
        res_ids = [r['id'] for r in self.state_data.get('resources', [])]
        self.max_p_combo['values'] = proc_ids
        self.event_p_combo['values'] = proc_ids
        self.max_r_combo['values'] = res_ids
        self.event_r_combo['values'] = res_ids
        
        self.deadlock_cycle = self.state_data.get('deadlock_cycle', [])
        is_deadlocked_now = bool(self.deadlock_cycle)
        
        if is_deadlocked_now and self.strategy_var.get() == "DETECT":
            self.recover_button.config(state=tk.NORMAL)
            if not self.flashing:
                self.flashing = True
                self.flash_deadlock_cycle()
        else:
            self.recover_button.config(state=tk.DISABLED)
            self.flashing = False
            
        self.is_deadlocked = is_deadlocked_now

        # 7. Redraw Graph only if no animation was triggered
        if not is_event_log and not is_aging_log:
            self.draw_graph()


    def draw_graph(self, flash_color=None):
        self.canvas.delete("all")
        if not self.state_data: return

        self.update_node_coords(self.state_data.get('processes', []), self.state_data.get('resources', []))

        # Draw Resources (Squares) - ATTRACTIVE ENHANCEMENTS
        for r in self.state_data.get('resources', []):
            rid = r['id']
            x, y = self.res_coords.get(rid, (0,0))
            tags = ("resource", f"res_{rid}")
            
            # --- Attractive Enhancement: Thicker, darker outline for impact ---
            rect_fill = "#EFEFEF" # Slightly darker inner color than canvas BG
            self.canvas.create_rectangle(x-20, y-20, x+20, y+20, fill=rect_fill, outline=NODE_OUTLINE, width=3, tags=tags)
            
            # Resource Status Bar (Bright Teal for capacity)
            if r['total'] > 0:
                percentage = r['available'] / r['total']
                bar_height = 40 * percentage
                
                # Draw the main bar
                self.canvas.create_rectangle(
                    x - 18, 
                    y + 20 - bar_height,
                    x + 18, 
                    y + 20, 
                    fill="#00BFA5", # Bright Teal for high visibility
                    outline=""
                )
                # Subtle white highlight line at the top of the filled portion
                if bar_height > 1:
                     self.canvas.create_line(x-18, y + 20 - bar_height, x+18, y + 20 - bar_height, fill="#FFFFFF", width=1)
                
            self.canvas.create_text(x, y, text=f"R{rid}\n({r['available']}/{r['total']})", fill=TEXT_COLOR, font=("Arial", 9, "bold"), tags=tags)

        # Draw Processes (Circles) - ATTRACTIVE ENHANCEMENTS
        for p in self.state_data.get('processes', []):
            pid = p['id']
            x, y = self.proc_coords.get(pid, (0,0))
            
            fill_color = NODE_GREEN
            if any(w['process_id'] == pid for w in self.state_data.get('waiting', [])):
                fill_color = NODE_YELLOW
                
            if pid in self.deadlock_cycle:
                fill_color = flash_color if flash_color is not None else NODE_RED
                
            tags = ("process", f"proc_{pid}")
            
            # --- Attractive Enhancement: Thicker outline and inner highlight ---
            # 1. Main circle with thicker outline
            self.canvas.create_oval(x-20, y-20, x+20, y+20, fill=fill_color, outline=NODE_OUTLINE, width=3, tags=tags)
            # 2. Inner highlight circle for 3D/depth effect
            self.canvas.create_oval(x-17, y-17, x+17, y+17, outline="#FFFFFF", width=1, tags=tags)
            # 3. Text
            self.canvas.create_text(x, y, text=f"P{pid}\nPrio: {p['priority']}", fill=TEXT_COLOR, font=("Arial", 9, "bold"), tags=tags)
        
        # Draw Arrows (Same as previous step)
        # Allocation (Resource -> Process)
        for p in self.state_data.get('processes', []):
            pid = p['id']
            for held in p.get('held', []):
                rid = held['id']
                if pid in self.proc_coords and rid in self.res_coords:
                    self.draw_arrow(self.res_coords[rid], self.proc_coords[pid], f"{held['count']}", ARROW_ALLOCATION, width=3)

        # Waiting (Process -> Resource)
        for w in self.state_data.get('waiting', []):
            pid = w['process_id']
            rid = w['resource_id']
            color = ARROW_WAITING
            if pid in self.deadlock_cycle:
                color = NODE_RED
                
            if pid in self.proc_coords and rid in self.res_coords:
                self.draw_arrow(self.proc_coords[pid], self.res_coords[rid], f"{w['count']}", color, width=1)

    def draw_arrow(self, p1, p2, text, color, width=2):
        x1, y1 = p1
        x2, y2 = p2
        self.canvas.create_line(x1, y1, x2, y2, arrow=tk.LAST, fill=color, width=width)
        self.canvas.create_text((x1+x2)/2, (y1+y2)/2 - 10, text=text, fill=TEXT_COLOR, font=("Arial", 9, "bold"))
        
    def update_node_coords(self, processes, resources):
        canvas_w = self.canvas.winfo_width()
        canvas_h = self.canvas.winfo_height()
        
        proc_x = canvas_w * 0.25
        for i, p in enumerate(processes):
            pid = p['id']
            if pid not in self.proc_coords or self.proc_coords[pid] == (0,0):
                y = (canvas_h / (len(processes) + 1)) * (i + 1)
                self.proc_coords[pid] = (proc_x, y)
                
        res_x = canvas_w * 0.75
        for i, r in enumerate(resources):
            rid = r['id']
            if rid not in self.res_coords or self.res_coords[rid] == (0,0):
                y = (canvas_h / (len(resources) + 1)) * (i + 1)
                self.res_coords[rid] = (res_x, y)

    # --- Drag and Drop (Same as original) ---
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
        
        tags = self.canvas.gettags(self._drag_data["item"])
        if "process" in tags:
            pid = [t.split("_")[1] for t in tags if t.startswith("proc_")][0]
            self.proc_coords[int(pid)] = (event.x, event.y)
        elif "resource" in tags:
            rid = [t.split("_")[1] for t in tags if t.startswith("res_")][0]
            self.res_coords[int(rid)] = (event.x, event.y)
        
        self.draw_graph()

    def on_closing(self):
        if self.cpp_process:
            self.cpp_process.terminate()
            self.cpp_process.wait()
        self.destroy()

# --- Run the App ---
if __name__ == "__main__":
    app = DeadlockApp()
    app.mainloop()