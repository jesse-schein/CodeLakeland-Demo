Write-Host "*****************" -ForegroundColor Red
Write-Host "Bundling Web Site" -ForegroundColor Red
Write-Host "*****************" -ForegroundColor Red
cd "$pwd\WebsiteSource"
webpack -p --display normal
cd ".."


Write-Host "*********************" -ForegroundColor Red
Write-Host "Creating Spiffs Image" -ForegroundColor Red
Write-Host "*********************" -ForegroundColor Red
Start-Sleep 3
./mkspiffs.exe -c ./data -p 256 -b 4096 -s 1503232 spiffs.bin

Write-Host "*****************" -ForegroundColor Red
Write-Host "Writing to Device" -ForegroundColor Red
Write-Host "*****************" -ForegroundColor Red
Start-Sleep 3

$USER = $env:USername
python "C:\Users\$USER\.platformio\packages\tool-esptoolpy\esptool.py" --port "COM3" write_flash 0x291000 spiffs.bin

Write-Host "***********************" -ForegroundColor Red
Write-Host "Finished Writing Device" -ForegroundColor Red
Write-Host "***********************" -ForegroundColor Red
Start-Sleep 1

Write-Host "**********************" -ForegroundColor Red
Write-Host "Opening Serial Monitor" -ForegroundColor Red
Write-Host "**********************" -ForegroundColor Red
Start-Sleep 1

pio device monitor --port COM3 --baud 115200
