import os
import sys
import serial
from datetime import datetime

if len(sys.argv)!=2:
    print("Usage: python upgrade.py file.bin")
    sys.exit(0)

length=os.path.getsize(sys.argv[1])
bytesRead=0
start=datetime.now()
with serial.Serial('COM25', 115200, timeout=2000) as ser:
    ser.write(f"o{{ota={length}}}o".encode("utf-8"))

    with open(sys.argv[1], mode="rb") as file:
        while chunk:=file.read(4096):
            bytesRead+=len(chunk)
            print(f"{int(bytesRead*100/length)}%\r",end="")
            sys.stdout.flush()
            ser.write(chunk)
print('')
print(f"Update took {(datetime.now()-start).total_seconds()} seconds")
