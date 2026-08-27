# BTC Purchase Tracker — C++/Qt 6

Tracker desktop locale per acquisti periodici di Bitcoin.

## Funzioni incluse

- Data acquisto
- Sito / exchange
- Euro spesi
- BTC on-chain
- Satoshi
- TX / ID transazione
- Totali euro / BTC / satoshi
- Inserimento, modifica, eliminazione
- Importazione CSV con riconoscimento intestazioni e validazione
- Rilevamento duplicati tramite TX/ID non vuoto
- Esportazione CSV
- Esportazione PDF
- Backup database
- Scelta della cartella del database al primo avvio
- Possibilità di cambiare cartella in seguito senza cancellare la vecchia copia

## Precisione dei dati

Nel database:

- gli euro sono salvati come **centesimi interi** (`INTEGER`)
- Bitcoin è salvato come **satoshi interi** (`INTEGER`)

BTC è una rappresentazione calcolata di sats. Non vengono usati floating point per memorizzare Bitcoin.

## Database al primo avvio

Al primo avvio viene aperta una finestra che chiede dove conservare il database. Il file si chiama:

`btc-purchase-tracker.sqlite`

Il percorso scelto viene ricordato nelle impostazioni Qt dell'utente.

## CSV importabile

Sono riconosciute intestazioni comuni, tra cui:

- `Data`, `Date`, `Data acquisto`
- `Sito`, `Site`, `Exchange`, `Piattaforma`
- `Euro`, `EUR`, `Euro spesi`
- `BTC`, `Bitcoin`, `BTC on-chain`
- `Satoshi`, `Sats`
- `TX`, `TXID`, `ID transazione`

Sono accettati separatori `;`, `,` e tab. È sufficiente avere BTC **oppure** satoshi; se sono presenti entrambi devono coincidere.

Date riconosciute:

- `26/08/2026`
- `2026-08-26`
- `26-08-2026`

Prima dell'import viene mostrato un riepilogo con righe valide, duplicate e righe errate. Le righe valide vengono inserite in una transazione SQLite unica.

## Compilazione su Arch / EndeavourOS

Dipendenze di sviluppo tipiche:

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base
```

Poi:

```bash
./build.sh
./build/btc-purchase-tracker
```

## AppImage

Il progetto contiene `build_appimage.sh`. Dopo la compilazione scarica nella cartella locale `tools/` i tool portabili `linuxdeploy` e `linuxdeploy-plugin-qt`, quindi genera l'AppImage. Non installa quei tool nel sistema.

```bash
./build_appimage.sh
```

Per una AppImage destinata a molti sistemi è consigliabile costruirla su una distribuzione con glibc non troppo recente, così aumenta la compatibilità con distro più vecchie. Il sorgente resta identico.

## Compilazione AppImage con GitHub Actions (senza installare nulla sul PC)

Il repository contiene già la workflow:

`.github/workflows/appimage.yml`

Procedura:

1. Crea un repository GitHub e carica **il contenuto** di questa cartella nella radice del repository.
2. Apri la scheda **Actions** del repository.
3. Se GitHub chiede di abilitare Actions, abilitala per il repository.
4. Seleziona **Build AppImage**.
5. Premi **Run workflow** e conferma.
6. Al termine della build apri l'esecuzione completata e, nella sezione **Artifacts**, scarica `BTC-Purchase-Tracker-1.0.0-x86_64`.
7. Estrai lo ZIP dell'artifact: contiene:
   - `BTC-Purchase-Tracker-1.0.0-x86_64.AppImage`
   - `BTC-Purchase-Tracker-1.0.0-x86_64.AppImage.sha256`

Sul PC che deve eseguire l'app:

```bash
chmod +x BTC-Purchase-Tracker-1.0.0-x86_64.AppImage
./BTC-Purchase-Tracker-1.0.0-x86_64.AppImage
```

La compilazione avviene interamente su un runner Ubuntu 22.04 di GitHub Actions. Qt, CMake e il compilatore non vengono installati sul computer dell'utente.

### Verifica facoltativa del download

Nella stessa cartella dell'AppImage:

```bash
sha256sum -c BTC-Purchase-Tracker-1.0.0-x86_64.AppImage.sha256
```

Se il file è integro verrà mostrato `OK`.
