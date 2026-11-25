Key features summary:

- GNSS_BLE_WITHOUT_NTRIP
Use case: used with SW Maps
Use Ntrip in SW Maps

- GNSS_BT_WITH_NTRIP
Use case: used with SW Maps, Locus Gis, QField,... 
Ntrip calls are embedded in the esp32

- GNSS_BT_WITHOUT_NTRIP
Use case: 
  - used with SW Maps and Locus Gis
  - Or, on Android, use Lefebure Ntrip Client (and change Gnss settings in android developper settings to use Lefebure as GPS reference) if you want have centrimeter-level accuracy in QField,...
  You can also use SW Maps as centrimeter-level accuracy GPS provider like Lefebure Ntrip Client. For that, enable "Mock Location output" in SW Maps settings
Use Ntrip in SW Maps or Lefebure Ntrip Client 

