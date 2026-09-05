BTC Purchase Tracker - Windows portable patch

SOSTITUISCI:
  CMakeLists.txt

AGGIUNGI:
  windows/app.rc
  windows/btc_purchase_tracker_icon.ico
  .github/workflows/windows.yml

Non modificare src/: Linux e Windows usano gli stessi sorgenti.

Dopo il commit:
  GitHub -> Actions -> Build Windows Portable -> Run workflow

Artifact atteso:
  BTC-Purchase-Tracker-1.0.0-Windows-x64

Dentro troverai uno ZIP portabile.
L'utente Windows deve soltanto estrarlo e avviare:
  BTC-Purchase-Tracker.exe
