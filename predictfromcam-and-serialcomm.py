import cv2
from ultralytics import YOLO
import serial
import serial.tools.list_ports
import time
import numpy as np

class ESP32Communicator:
    def __init__(self, baud_rate=115200):
        self.esp32_port = self.find_esp32_port()
        if not self.esp32_port:
            print("⚠️  ESP32 not found! Will run in simulation mode (no serial comm)")
            self.ser = None
        else:
            try:
                self.ser = serial.Serial(
                    port=self.esp32_port,
                    baudrate=baud_rate,
                    timeout=1,
                    parity=serial.PARITY_NONE,
                    stopbits=serial.STOPBITS_ONE,
                    bytesize=serial.EIGHTBITS
                )
                time.sleep(2)  # Wait for connection to stabilize
                print(f"✅ Connected to ESP32 on {self.esp32_port}")
            except Exception as e:
                print(f"❌ Failed to connect: {e}")
                self.ser = None
    
    def find_esp32_port(self):
        """Automatically find ESP32 port (works on Windows/Linux/Mac)"""
        ports = serial.tools.list_ports.comports()
        for port in ports:
            # ESP32 common identifiers (CP210x, CH340, etc.)
            if any(id in port.description.lower() for id in ['cp210', 'ch340', 'silicon', 'esp32']):
                return port.device
            # For Mac users - check tty.usb*
            if 'usb' in port.device.lower():
                return port.device
        return None
    
    def send_plastic_type(self, plastic_type, confidence):
        """Send plastic type detection result to ESP32"""
        if self.ser and self.ser.is_open:
            # Format: "PLASTIC_TYPE:CONFIDENCE\n" 
            # Example: "MLP:0.95\n"
            message = f"{plastic_type}:{confidence:.2f}\n"
            try:
                self.ser.write(message.encode())
                print(f"📤 Sent: {message.strip()}")
                return True
            except Exception as e:
                print(f"❌ Send failed: {e}")
                return False
        else:
            # Simulation mode
            print(f"🔷 [SIMULATION] Would send: {plastic_type}:{confidence:.2f}")
            return False
    
    def close(self):
        """Close serial connection"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("🔌 Serial connection closed")

def main():
    # 1. Initialize ESP32 communication
    print("🔄 Initializing ESP32 communication...")
    esp32 = ESP32Communicator(baud_rate=115200)  # Adjust baud rate if needed
    
    # 2. Load your trained YOLO11 plastic detection model
    print("🔄 Loading YOLO model...")
    model = YOLO('./trainresults/weights/best.pt')
    
    # 3. Initialize the webcam
    cap = cv2.VideoCapture(0)
    
    if not cap.isOpened():
        print("Error: Could not open webcam.")
        exit()
    
    print("\n✅ System ready!")
    print("📷 Webcam started. Press 'q' to exit.")
    print("🔍 Detecting plastic types: milk packet, color LD, MLP, PP\n")
    
    # Track last sent detection to avoid spamming the same detection
    last_sent = {"type": None, "time": 0}
    send_cooldown = 2.0  # seconds between sending same plastic type
    
    while True:
        # Capture frame-by-frame
        success, frame = cap.read()
        
        if not success:
            break
        
        # Run YOLO11 inference on the frame
        results = model.predict(source=frame, conf=0.5, show=False, stream=True)
        
        # Process results
        detected_plastics = []
        for r in results:
            # Get detection info
            boxes = r.boxes
            for box in boxes:
                # Get class name and confidence
                class_id = int(box.cls[0])
                class_name = model.names[class_id]
                confidence = float(box.conf[0])
                
                # Check if it's one of your target plastic types
                # Adjust these names based on your model's actual class names
                if class_name.lower() in ['milk packet', 'color ld', 'mlp', 'pp']:
                    detected_plastics.append({
                        'type': class_name.upper(),
                        'confidence': confidence
                    })
            
            # Annotate frame
            annotated_frame = r.plot()
        
        # Send detection to ESP32 (prioritize highest confidence detection)
        if detected_plastics:
            # Get highest confidence detection
            best_detection = max(detected_plastics, key=lambda x: x['confidence'])
            
            # Check if we should send (cooldown and different from last)
            current_time = time.time()
            if (best_detection['type'] != last_sent['type'] or 
                current_time - last_sent['time'] > send_cooldown):
                
                # Send to ESP32
                esp32.send_plastic_type(
                    best_detection['type'], 
                    best_detection['confidence']
                )
                
                # Update last sent
                last_sent['type'] = best_detection['type']
                last_sent['time'] = current_time
            
            # Add detection info to frame
            info_text = f"Detected: {best_detection['type']} ({best_detection['confidence']:.2f})"
            cv2.putText(annotated_frame, info_text, (10, 30), 
                       cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        
        # Display the frame
        cv2.imshow("YOLO11 Plastic Type Detection", annotated_frame)
        
        # Break the loop if 'q' is pressed
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    
    # Clean up
    cap.release()
    cv2.destroyAllWindows()
    esp32.close()
    print("\n👋 Program terminated")

if __name__ == "__main__":
    main()