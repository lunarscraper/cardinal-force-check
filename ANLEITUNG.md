# Cardinal-Check auf der Force ausführen

Der Test ist **nur lesend**: Er installiert nichts, ändert keine Einstellungen und stoppt MPC nicht.

1. Den Ordner `cardinal-check` per Termius (SFTP) nach **`/sdcard/`** kopieren.
   Nicht nach `/tmp`: der Ordner ist groß, und `/tmp` liegt im Arbeitsspeicher.
2. Im Termius-Terminal eingeben:

       sh /sdcard/cardinal-check/check.sh

   Das ist Stufe 1 und dauert je nach SD-Karte bis zu einer Minute.
3. Nur wenn Stufe 1 „geladen“ meldet: **MPC-Projekt speichern**, dann

       sh /sdcard/cardinal-check/check.sh --bench

4. Die Datei `/sdcard/cardinal-check/report.txt` (per Termius herunterladen)
   zusammen mit `analysis.txt` an Claude schicken.

Aufräumen danach: den Ordner `/sdcard/cardinal-check` einfach löschen.
