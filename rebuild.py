import subprocess, os, sys

APPS = ["Diceware","EpubReader","GraphicsDemo","MystifyDemo","SerialConsole","Snake","TamaTac","TwoEleven"]

sdk = r"C:\Tactility\release\TactilitySDK\0.8.0-ESP32-C5-dev-esp32c5\TactilitySDK"
idf_path = os.environ.get("IDF_PATH", r"C:\esp\v6.1\esp-idf")
idf_py = os.path.join(idf_path, "tools", "idf.py")

env = os.environ.copy()
env["TACTILITY_SDK_PATH"] = sdk

results = {}
for app in APPS:
    d = os.path.join("Apps", app)
    print(f"\n===== {app} (fullclean) =====", flush=True)
    subprocess.run([sys.executable, idf_py, "-B", "build", "fullclean"],
                   cwd=d, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print(f"===== {app} (build) =====", flush=True)
    r = subprocess.run(
        [sys.executable, idf_py, "-B", "build", "build"],
        cwd=d, env=env,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
    )
    tail = r.stdout.strip().splitlines()[-12:]
    print("\n".join(tail), flush=True)
    elf = os.path.join(d, "build", f"tactility.{app.lower()}.app.elf")
    ok = os.path.exists(elf)
    results[app] = (r.returncode, ok)
    print(f"[{app}] rc={r.returncode} elf_present={ok}", flush=True)

print("\n===== SUMMARY =====", flush=True)
for app, (rc, ok) in results.items():
    print(f"{app}: rc={rc} elf={ok}", flush=True)
