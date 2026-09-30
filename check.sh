#!/bin/sh
# Cardinal-Machbarkeitstest für die Akai Force. NUR LESEND:
# installiert nichts, ändert keine Einstellungen, stoppt MPC nicht.
#
#   sh check.sh           Geräte-Infos + Stufe 1 (lässt sich Cardinal überhaupt laden?)
#   sh check.sh --bench   zusätzlich Stufe 2 (Cardinal starten, 10 s Audio, CPU/Speicher messen)
#                         Vorher das MPC-Projekt speichern: Stufe 2 braucht viel Arbeitsspeicher.
#
# Das Ergebnis steht am Ende auch in report.txt im selben Ordner.

DIR=$(cd "$(dirname "$0")" && pwd)
REPORT="$DIR/report.txt"
PLUGIN=$(find "$DIR/plugin" -name '*.so' 2>/dev/null | head -n 1)

run() {
echo "===== Cardinal-Check $(date '+%Y-%m-%d %H:%M')"
echo
echo "== Gerät"
echo "Architektur: $(uname -m), Kernel $(uname -r)"
grep -m1 -i 'hardware' /proc/cpuinfo
echo "CPU-Kerne: $(grep -c '^processor' /proc/cpuinfo)"
grep -E 'MemTotal|MemAvailable' /proc/meminfo
LIBC=$(find /lib /usr/lib -maxdepth 2 -name 'libc.so.6' 2>/dev/null | head -n 1)
[ -n "$LIBC" ] && echo "glibc: $("$LIBC" 2>/dev/null | head -n 1)"
echo "Freier Platz im Prüfordner:"
df -h "$DIR" | tail -n 1
echo "MPC läuft: $(pidof MPC >/dev/null && echo ja || echo nein)"
echo

echo "== Benötigte Bibliotheken (aus analysis.txt) auf dem Gerät"
if [ -f "$DIR/needed.txt" ]; then
    while read -r lib; do
        [ -z "$lib" ] && continue
        hit=$(find /lib /usr/lib /usr/local/lib -name "$lib" 2>/dev/null | head -n 1)
        if [ -n "$hit" ]; then echo "  ok      $lib ($hit)"; else echo "  FEHLT   $lib"; fi
    done < "$DIR/needed.txt"
else
    echo "  needed.txt nicht gefunden"
fi
echo

if [ -z "$PLUGIN" ]; then
    echo "FEHLER: keine Plugin-Datei (*.so) im Ordner plugin/ gefunden"
    return
fi
echo "Plugin: $PLUGIN ($(du -h "$PLUGIN" | cut -f1))"
# Das Prüfprogramm nach /tmp kopieren: auf der SD-Karte sind Programme evtl. nicht ausführbar
cp "$DIR/probe" /tmp/cardinal-probe && chmod +x /tmp/cardinal-probe
if [ "$1" = "--bench" ]; then
    /tmp/cardinal-probe "$PLUGIN" --bench 2>&1
else
    /tmp/cardinal-probe "$PLUGIN" 2>&1
fi
echo "Rückgabewert Prüfprogramm: $?"
rm -f /tmp/cardinal-probe
}

run "$1" 2>&1 | tee "$REPORT"
echo
echo "Fertig. Bericht gespeichert in: $REPORT"
