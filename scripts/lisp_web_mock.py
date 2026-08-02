#!/usr/bin/env python3
"""Local mock of the head unit's HTTP API, for working on the web LISP editor
without flashing (or even owning) a board.

Serves main/web/lisp_editor.html at /lisp and fakes everything it talks to:

  /lisp/api/{state,read,code,upload,run,repl,console,console/clear,stats}
  /files/api/{list,download,upload,rename,delete,mkdir}

The "VESC" is a file in the sandbox directory; uploads and reads crawl through
a simulated progress curve so the progress bar and polling can be exercised,
and a background thread prints to the console the way a running script would.

    scripts/lisp_web_mock.py [--port 8080] [--dir /tmp/lisp_mock]
    open http://localhost:8080/lisp
"""

import argparse
import json
import os
import shutil
import socketserver
import threading
import time
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PAGE = os.path.join(ROOT, "main", "web", "lisp_editor.html")

# --- fake device state -------------------------------------------------------

class Device:
    def __init__(self, sandbox):
        self.sandbox = sandbox
        self.code_path = os.path.join(sandbox, "_vesc_code.lisp")
        self.lock = threading.Lock()
        self.job = dict(active=False, finished=False, kind=0, result=0,
                        done=0, total=0)
        self.console = []      # list of (seq, kind, text)
        self.seq = 0
        self.dropped = 0
        self.running = False
        if not os.path.exists(self.code_path):
            with open(self.code_path, "w") as f:
                f.write('; code "stored on the VESC"\n(defun hi () (print "hi"))\n')

    def log(self, text, kind="lisp"):
        with self.lock:
            self.seq += 1
            self.console.append((self.seq, kind, text))
            if len(self.console) > 192:
                self.console.pop(0)
                self.dropped += 1

    def _run_job(self, kind, payload=None, run_after=False):
        total = len(payload) if payload is not None else os.path.getsize(self.code_path)
        total = max(total, 1)
        self.job.update(active=True, finished=False, kind=kind, result=0,
                        done=0, total=total)

        def work():
            step = max(total // 20, 64)
            done = 0
            while done < total:
                time.sleep(0.15)
                done = min(total, done + step)
                self.job["done"] = done
            if kind == 1 and payload is not None:
                with open(self.code_path, "w") as f:
                    f.write(payload)
                if run_after:
                    self.running = True
                    self.log("script started", "note")
            self.job.update(active=False, finished=True, result=0)

        threading.Thread(target=work, daemon=True).start()

    def start_read(self):
        if self.job["active"]:
            return False
        self._run_job(2)
        return True

    def start_upload(self, code, run_after):
        if self.job["active"]:
            return False
        self._run_job(1, code, run_after)
        return True

    def read_code(self):
        with open(self.code_path) as f:
            return f.read()


DEV = None

# --- sandbox path mapping ("/vescfs/x" -> <sandbox>/vescfs/x) -----------------

def to_real(dev, path):
    p = os.path.normpath("/" + path.lstrip("/"))
    if ".." in p.split("/"):
        raise ValueError("bad path")
    return os.path.join(dev.sandbox, p.lstrip("/"))


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, fmt, *args):
        print("  %s %s" % (self.command, self.path))

    # -- helpers --
    def send(self, code, body=b"", ctype="application/json"):
        if isinstance(body, str):
            body = body.encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def jsend(self, obj, code=200):
        self.send(code, json.dumps(obj))

    def err(self, code, msg):
        self.jsend({"error": msg}, code)

    def query(self):
        q = urllib.parse.urlparse(self.path).query
        return {k: v[0] for k, v in urllib.parse.parse_qs(q).items()}

    def body(self):
        n = int(self.headers.get("Content-Length", 0))
        return self.rfile.read(n) if n else b""

    # -- routing --
    def do_GET(self):
        p = urllib.parse.urlparse(self.path).path
        q = self.query()
        if p in ("/", "/lisp"):
            with open(PAGE, "rb") as f:
                return self.send(200, f.read(), "text/html")
        if p == "/lisp/api/state":
            j = DEV.job
            out = dict(busy=j["active"], mine=j["active"], kind=j["kind"],
                       done=j["done"], total=j["total"], finished=j["finished"],
                       result=j["result"],
                       msg=("Uploaded" if j["kind"] == 1 else "Read from VESC")
                           if j["finished"] else "",
                       codeLen=len(DEV.read_code()), codeMax=110 * 1024,
                       replMax=240, consoleSeq=DEV.seq, consoleDropped=DEV.dropped,
                       stats=dict(cpu=12.5, heap=44.0, mem=31.2, stack=18.0,
                                  vars=[{"n": "speed", "v": 21.4},
                                        {"n": "mode", "v": 2}]))
            return self.jsend(out)
        if p == "/lisp/api/code":
            return self.send(200, DEV.read_code(), "text/plain; charset=utf-8")
        if p == "/lisp/api/console":
            since = int(q.get("since", 0))
            with DEV.lock:
                lines = [{"s": s, "k": k, "t": t} for s, k, t in DEV.console if s > since]
                seq = DEV.console[-1][0] if DEV.console else since
            return self.jsend({"lines": lines, "seq": seq,
                               "dropped": DEV.dropped, "more": False})
        if p == "/files/api/list":
            return self.files_list(q.get("path", "/"))
        if p == "/files/api/download":
            try:
                with open(to_real(DEV, q["path"]), "rb") as f:
                    return self.send(200, f.read(), "application/octet-stream")
            except Exception as e:
                return self.err(404, str(e))
        return self.err(404, "not found")

    def do_POST(self):
        p = urllib.parse.urlparse(self.path).path
        q = self.query()
        if p == "/lisp/api/read":
            return self.jsend({"ok": True}) if DEV.start_read() else self.err(409, "busy")
        if p == "/lisp/api/upload":
            code = self.body().decode("utf-8", "replace")
            ok = DEV.start_upload(code, q.get("run", "1") != "0")
            return self.jsend({"ok": True}) if ok else self.err(409, "busy")
        if p == "/lisp/api/run":
            DEV.running = q.get("run", "1") != "0"
            DEV.log("script %s" % ("started" if DEV.running else "stopped"), "note")
            return self.jsend({"ok": True})
        if p == "/lisp/api/repl":
            expr = self.body().decode("utf-8", "replace")
            if len(expr) > 240:
                return self.err(413, "too large")
            DEV.log("> %s" % expr, "note")
            DEV.log("=> %s" % (len(expr)), "lisp")
            return self.jsend({"ok": True})
        if p == "/lisp/api/console/clear":
            with DEV.lock:
                DEV.console.clear()
            return self.jsend({"ok": True})
        if p == "/lisp/api/stats":
            return self.jsend({"ok": True})
        if p == "/files/api/upload":
            try:
                real = to_real(DEV, q["path"])
                os.makedirs(os.path.dirname(real), exist_ok=True)
                with open(real, "wb") as f:
                    f.write(self.body())
                return self.jsend({"ok": True})
            except Exception as e:
                return self.err(500, str(e))
        if p in ("/files/api/rename", "/files/api/delete", "/files/api/mkdir"):
            try:
                j = json.loads(self.body() or b"{}")
                if p.endswith("rename"):
                    shutil.move(to_real(DEV, j["src"]), to_real(DEV, j["dst"]))
                elif p.endswith("delete"):
                    real = to_real(DEV, j["path"])
                    shutil.rmtree(real) if os.path.isdir(real) else os.remove(real)
                else:
                    os.makedirs(to_real(DEV, j["path"]), exist_ok=True)
                return self.jsend({"ok": True})
            except Exception as e:
                return self.err(500, str(e))
        return self.err(404, "not found")

    def files_list(self, path):
        if path == "/":
            return self.jsend({"path": "/", "root": "/", "entries": [
                {"name": "vescfs", "dir": True, "size": 0},
                {"name": "sdcard", "dir": True, "size": 0}]})
        try:
            real = to_real(DEV, path)
            os.makedirs(real, exist_ok=True)
            entries = []
            for name in sorted(os.listdir(real)):
                full = os.path.join(real, name)
                entries.append({"name": name, "dir": os.path.isdir(full),
                                "size": 0 if os.path.isdir(full) else os.path.getsize(full)})
            return self.jsend({"path": path.rstrip("/") or "/", "root": "/",
                               "entries": entries})
        except Exception as e:
            return self.err(404, str(e))


class Server(ThreadingHTTPServer):
    """HTTPServer.server_bind() calls socket.getfqdn(), which blocks on a
    reverse DNS lookup for tens of seconds on a machine with no resolver
    (or one behind a sandbox). We only need the address for logging."""
    allow_reuse_address = True
    daemon_threads = True

    def server_bind(self):
        socketserver.TCPServer.server_bind(self)
        self.server_name, self.server_port = self.server_address[:2]


def chatter():
    """Stand-in for a running script printing from a loop."""
    n = 0
    while True:
        time.sleep(2.0)
        if DEV.running:
            n += 1
            DEV.log("tick %d  rpm=%d" % (n, 1200 + n % 300))


def main():
    global DEV
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=8080)
    ap.add_argument("--dir", default="/tmp/lisp_mock")
    a = ap.parse_args()

    os.makedirs(os.path.join(a.dir, "vescfs", "lisp"), exist_ok=True)
    os.makedirs(os.path.join(a.dir, "sdcard"), exist_ok=True)
    DEV = Device(a.dir)
    DEV.log("mock device ready", "note")
    threading.Thread(target=chatter, daemon=True).start()

    srv = Server(("0.0.0.0", a.port), Handler)
    print("mock head unit on http://localhost:%d/lisp  (sandbox %s)" % (a.port, a.dir))
    srv.serve_forever()


if __name__ == "__main__":
    main()
