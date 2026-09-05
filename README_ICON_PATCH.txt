BTC PURCHASE TRACKER - PATCH ICONA

Copia/sostituisci nel repository GitHub:

1) assets/btc_purchase_tracker_icon.png
2) assets/btc-purchase-tracker.desktop
3) appimage/btc-purchase-tracker.desktop
4) resources/resources.qrc   (file nuovo)
5) src/main.cpp
6) CMakeLists.txt
7) .github/workflows/appimage.yml

Poi Commit changes e rilancia:
Actions -> Build AppImage -> Run workflow

L'icona viene usata:
- nell'AppImage/menu desktop tramite linuxdeploy + .desktop
- nella finestra Qt tramite resources.qrc incorporato nell'eseguibile
