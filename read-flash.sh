#running partition: type 0, subtype 16, address 10000, label app0
#sha256: 22d4d82cb942c455d83c1694d02f9be7141288c976ccd57c2ed50de280716ce5

#app partitions:
#type 0, subtype 16, address 10000, label app0
#sha256: 22d4d82cb942c455d83c1694d02f9be7141288c976ccd57c2ed50de280716ce5

#type 0, subtype 17, address 150000, label app1
#sha256: 22d4d82cb942c455d83c1694d02f9be7141288c9e8170e80601efb3f8c85fb3f

#data partitions:
#type 1, subtype 2, address 9000, label nvs
#sha256: 8d0aac9fd329ae878666c76c90c6e67e6e7af1d9c3993740c36dc1109414d28b

#type 1, subtype 0, address e000, label otadata
#sha256: f94c5d786a7a8fab06ac5d10e33bf37711a6697636dc037559ea19cc410a17f0

#type 1, subtype 153, address 290000, label eeprom
#sha256: f47a8ec3e9aff2318d896942282ad4fe37d6391c82914f54a5da8a37de1300c6

#type 1, subtype 130, address 291000, label spiffs
#sha256: fb9ce2d550f312e7e88ca5dff525bbdff9c9db1f89cb9c47cb615105bbc693db

# Read the current contents of the EEPROM - this contains the wifi network info (useful for factory programming)
esptool.py -b 921600 read_flash 0x290000 0x1000 eeprom.bin
esptool.py -b 921600 read_flash 0x9000 0x5000 nvs.bin