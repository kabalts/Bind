Write-Host "Starting server and client..." -ForegroundColor Green
Start-Process "out/build/x64-Debug/bin/bind_server.exe"
Start-Process "out/build/x64-Debug/bin/bind_client.exe"
