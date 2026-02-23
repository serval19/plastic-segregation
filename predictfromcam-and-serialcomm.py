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
    
    def get_plastic_code(self, plastic_type):
        """Convert plastic type name to numeric code"""
        # Convert to lowercase for case-insensitive matching
        plastic_lower = plastic_type.lower()
        
        # Mapping of plastic types to numeric codes
        if 'milk packet' in plastic_lower:
            return 1
        elif 'color ld' in plastic_lower or 'colorld' in plastic_lower:
            return 2
        elif 'mlp' in plastic_lower:
            return 3
        elif 'pp' in plastic_lower:
            return 4
        else:
            return 0  # Unknown type
    
    def send_plastic_code(self, plastic_type, confidence):
        """Send plastic type detection result as numeric code to ESP32"""
        # Get numeric code for the plastic type
        plastic_code = self.get_plastic_code(plastic_type)
        
        if plastic_code == 0:
            print(f"⚠️ Unknown plastic type: {plastic_type}")
            return False
        
        if self.ser and self.ser.is_open:
            # Format: Just send the number as bytes, followed by newline
            # Example: "1\n" for milk packet, "2\n" for color ld, etc.
            message = f"{plastic_code}\n"
            try:
                self.ser.write(message.encode())
                print(f"📤 Sent code {plastic_code} for {plastic_type} (confidence: {confidence:.2f})")
                return True
            except Exception as e:
                print(f"❌ Send failed: {e}")
                return False
        else:
            # Simulation mode
            print(f"🔷 [SIMULATION] Would send code {plastic_code} for {plastic_type} (confidence: {confidence:.2f})")
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
    
    # 3. Initialize the webcam - MODIFIED FOR EXTERNAL WEBCAM
    print("📷 Looking for external webcam...")
    
    # Try different indices to find your external webcam
    # Usually: 0 = built-in webcam, 1 = first external webcam, 2 = second external webcam, etc.
    camera_index = 1  # Try 1 first, if not working try 2, etc.
    
    cap = None
    while camera_index < 5:  # Try up to index 4
        cap = cv2.VideoCapture(camera_index)
        if cap.isOpened():
            # Test if we can read a frame
            ret, test_frame = cap.read()
            if ret and test_frame is not None:
                print(f"✅ External webcam found at index {camera_index}")
                break
            else:
                cap.release()
                camera_index += 1
        else:
            camera_index += 1
    
    if not cap or not cap.isOpened():
        print("⚠️ External webcam not found, trying default camera (index 0)...")
        cap = cv2.VideoCapture(0)
        
        if not cap.isOpened():
            print("❌ Error: Could not open any webcam.")
            exit()
    
    # Optional: Set camera properties for external webcam
    # You can adjust these based on your external webcam's capabilities
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)   # Set width
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)  # Set height
    cap.set(cv2.CAP_PROP_FPS, 30)             # Set FPS
    
    print("\n✅ System ready!")
    print("📷 Webcam started. Press 'q' to exit.")
    print("🔍 Detecting plastic types: milk packet, color LD, MLP, PP")
    print("📤 Sending codes to ESP32: 1=Milk Packet, 2=Color LD, 3=MLP, 4=PP\n")
    
    # Track last sent detection to avoid spamming the same detection
    last_sent = {"code": None, "time": 0}
    send_cooldown = 2.0  # seconds between sending same plastic type
    
    while True:
        # Capture frame-by-frame
        success, frame = cap.read()
        
        if not success:
            print("⚠️ Failed to grab frame, retrying...")
            continue
        
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
                        'type': class_name,
                        'confidence': confidence
                    })
            
            # Annotate frame
            annotated_frame = r.plot()
        
        # Send detection to ESP32 (prioritize highest confidence detection)
        if detected_plastics:
            # Get highest confidence detection
            best_detection = max(detected_plastics, key=lambda x: x['confidence'])
            
            # Get the numeric code for this detection
            current_code = esp32.get_plastic_code(best_detection['type'])
            
            # Check if we should send (cooldown and different from last)
            current_time = time.time()
            if (current_code != last_sent['code'] or 
                current_time - last_sent['time'] > send_cooldown):
                
                # Send to ESP32
                esp32.send_plastic_code(
                    best_detection['type'], 
                    best_detection['confidence']
                )
                
                # Update last sent
                last_sent['code'] = current_code
                last_sent['time'] = current_time
            
            # Add detection info to frame (show both type and code)
            info_text = f"Detected: {best_detection['type']} (Code: {current_code}) [{best_detection['confidence']:.2f}]"
            cv2.putText(annotated_frame, info_text, (10, 30), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        
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