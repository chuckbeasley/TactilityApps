import re, os

APPS = [
    "Apps/GraphicsDemo/main/Source/Main.cpp",
    "Apps/Diceware/main/Source/Diceware.cpp",
    "Apps/SerialConsole/main/Source/ConnectView.cpp",
    "Apps/TwoEleven/main/Source/TwoEleven.cpp",
    "Apps/EpubReader/main/Source/EpubReader.cpp",
    "Apps/MystifyDemo/main/Source/Main.cpp",
    "Apps/Snake/main/Source/Snake.cpp",
    "Apps/TamaTac/main/Source/TamaTac.cpp",
]

# app_manager_start_for_result( ID, PARENT, ARGC, ARGV, OUT )
#   -> app_start_for_result( ID, ARGC, ARGV, PARENT, OUT )
pattern = re.compile(
    r'app_manager_start_for_result\(\s*'
    r'([^,]+),\s*'      # ID
    r'([^,]+),\s*'      # PARENT
    r'([^,]+),\s*'      # ARGC
    r'([^,]+),\s*'      # ARGV
    r'([^)]+)\)',       # OUT
    re.DOTALL
)

def repl(m):
    id_, parent, argc, argv_, out = m.groups()
    return f'app_start_for_result({id_}, {argc}, {argv_}, {parent}, {out})'

for f in APPS:
    with open(f, encoding='utf-8') as fh:
        text = fh.read()

    new = pattern.sub(repl, text)
    # Fix short app ids -> upstream tactility.* ids (only in the call args we just rewrote)
    new = new.replace('app_start_for_result("AlertDialog"', 'app_start_for_result("tactility.alertdialog"')
    new = new.replace('app_start_for_result("SelectionDialog"', 'app_start_for_result("tactility.selectiondialog"')

    # Ensure app/start.h is included (needed for app_start_for_result)
    if '#include <app/start.h>' not in new:
        if '#include <app/manager.h>' in new:
            new = new.replace('#include <app/manager.h>', '#include <app/manager.h>\n#include <app/start.h>', 1)
        elif '#include <app/manifest.h>' in new:
            new = new.replace('#include <app/manifest.h>', '#include <app/manifest.h>\n#include <app/start.h>', 1)

    if new != text:
        with open(f, 'w', encoding='utf-8', newline='\n') as fh:
            fh.write(new)
        print(f"UPDATED {f}")
    else:
        print(f"UNCHANGED {f}")
