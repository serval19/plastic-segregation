import serial
import time

# Test Arduino communication
try:
    ser = serial.Serial('COM3', 9600, timeout=1)
    time.sleep(2)
    print("✅ Connected to Arduino on COM3")
    
    # Send test codes
    for code in [1, 2, 3, 4]:
        print(f"Sending code {code}...")
        ser.write(f"{code}\n".encode())
        time.sleep(1)
        
        # Check for response
        if ser.in_waiting:
            response = ser.readline().decode().strip()
            print(f"Response: {response}")
    
    ser.close()
    print("Test complete!")
    
except Exception as e:
    print(f"❌ Error: {e}")