# Výprava za pribináčkem verze 1.5.0
Velice vtipná hra o cestě pro pribináčka

## Stažení
- [Itch.io](https://penk-studios.itch.io/vyprava-za-pribinackem-lite)
- [Google Play](https://play.google.com/store/apps/details?id=com.zahon.pribinacek)

## Přídáno
- Opraven bug, kdy na windowsu nefungoval zvuk 🤦

## Bugy
<span style="color: #ff5555">1.</span> Možný buffer overflow (nejspíše) u multiplayeru (prvně zpozorován v 1.4.0)

   <img src="bugs/01-serverlog.png" width="150" height="100" />
   <img src="bugs/01-subjekt1.png" width="150" height="100" />
   <img src="bugs/01-subjekt2.png" width="150" height="100" />
   <img src="bugs/01-pozorovatel1.png" width="150" height="100" />

   Popis: Normální setup s běžícím serverem, hráč hostne hru, druhý se připojí <br>
   Detaily: Zabugovaný klient byl ten, co se připojil. Kamera vypadá, jakoby byla "offsetnutá" od pozice hráče (kolize fungují jako kdyby byl hráč na pozici podle pozorovatele, stejně jako podle obrázku na tom je i "baterka" hráče). Funguje i po opuštění hry.

<span style="color: #ff5555">2.</span> Scaling potíže s safem (tlačítka)

<span style="color: #ff5555">3.</span> Mobil lze brát přes safe

## Potřeby
- [raylib](https://github.com/raysan5/raylib)
- [enet](http://enet.bespin.org/Downloads.html)
- [mingw-std-thread](https://github.com/meganz/mingw-std-threads) (pouze na windows)

-------------
## Buildování
### Linux
`# make linux`

### Windows
`# make windows`
