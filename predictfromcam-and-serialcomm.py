import cv2
from ultralytics import YOLO
import serial
import serial.tools.list_ports
import time

class ArduinoCommunicator:
    def __init__(self, baud_rate=9600):
        # Since we know it's COM3 from your test, use it directly
        self.arduino_port = 'COM3'  # Directly specify COM3
        
        try:
            self.ser = serial.Serial(
                port=self.arduino_port,
                baudrate=baud_rate,
                timeout=1,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS
            )
            time.sleep(2)  # Wait for connection to stabilize
            
            # Test communication
            self.ser.write(b"TEST\n")
            time.sleep(0.5)
            
            print(f"✅ Successfully connected to Arduino on {self.arduino_port}")
            
        except Exception as e:
            print(f"❌ Failed to connect: {e}")
            print("⚠️ Running in simulation mode")
            self.ser = None
    
    def get_plastic_code(self, plastic_type):
        """Convert plastic type name to numeric code"""
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
            return 0
    
    def send_plastic_code(self, plastic_type, confidence):
        """Send plastic type code to Arduino"""
        plastic_code = self.get_plastic_code(plastic_type)
        
        if plastic_code == 0:
            print(f"⚠️ Unknown plastic type: {plastic_type}")
            return False
        
        if self.ser and self.ser.is_open:
            try:
                # Send the code
                message = f"{plastic_code}\n"
                self.ser.write(message.encode())
                print(f"📤 Sent code {plastic_code} to Arduino for {plastic_type} (confidence: {confidence:.2f})")
                
                # Optional: Read response
                time.sleep(0.1)
                if self.ser.in_waiting:
                    response = self.ser.readline().decode().strip()
                    print(f"📥 Arduino response: {response}")
                
                return True
            except Exception as e:
                print(f"❌ Send failed: {e}")
                return False
        else:
            print(f"🔷 [SIMULATION] Would send code {plastic_code} for {plastic_type}")
            return False
    
    def close(self):
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("🔌 Serial connection closed")

def find_webcam():
    """Find available webcam"""
    print("📷 Looking for webcam...")
    
    # Try external camera indices first
    for i in range(1, 5):
        cap = cv2.VideoCapture(i)
        if cap.isOpened():
            ret, test_frame = cap.read()
            if ret and test_frame is not None:
                print(f"✅ External webcam found at index {i}")
                return cap
            cap.release()
    
    # Fallback to built-in camera
    print("⚠️ External webcam not found, trying default camera (index 0)...")
    cap = cv2.VideoCapture(0)
    if cap.isOpened():
        return cap
    
    return None

def main():
    print("="*50)
    print("🤖 PLASTIC SEGREGATION SYSTEM")
    print("="*50)
    
    # 1. Initialize Arduino communication (now using COM3)
    print("\n🔄 Connecting to Arduino on COM3...")
    arduino = ArduinoCommunicator(baud_rate=9600)
    
    # 2. Load YOLO model
    print("\n🔄 Loading YOLO model...")
    try:
        model = YOLO('./trainresults/weights/best.pt')
        print("✅ YOLO model loaded successfully")
        print(f"📋 Model classes: {model.names}")
    except Exception as e:
        print(f"❌ Error loading model: {e}")
        exit()
    
    # 3. Initialize webcam
    cap = find_webcam()
    if cap is None:
        print("❌ Error: Could not open any webcam.")
        exit()
    
    # Set camera properties
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    
    print(f"\n✅ System ready!")
    print(f"📷 Camera: {int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))}x{int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))}")
    print(f"🔍 Detecting: milk packet, color LD, MLP, PP")
    print(f"📤 Sending codes to Arduino: 1=Milk Packet, 2=Color LD, 3=MLP, 4=PP")
    print(f"❌ Press 'q' to exit\n")
    
    # Tracking variables
    last_sent_code = None
    last_sent_time = 0
    send_cooldown = 3.0  # Don't send same code more than once every 3 seconds
    
    while True:
        # Capture frame
        success, frame = cap.read()
        if not success:
            print("⚠️ Failed to grab frame, retrying...")
            continue
        
        # Run YOLO inference
        results = model.predict(source=frame, conf=0.5, show=False, verbose=False)
        
        # Process results
        detected_plastics = []
        for r in results:
            boxes = r.boxes
            for box in boxes:
                class_id = int(box.cls[0])
                class_name = model.names[class_id]
                confidence = float(box.conf[0])
                
                # Check if it's one of target plastic types
                if class_name.lower() in ['milk packet', 'color ld', 'mlp', 'pp']:
                    detected_plastics.append({
                        'type': class_name,
                        'confidence': confidence
                    })
            
            annotated_frame = r.plot()
        
        # Send detection to Arduino if plastic detected
        if detected_plastics:
            # Get highest confidence detection
            best_detection = max(detected_plastics, key=lambda x: x['confidence'])
            
            # Get numeric code
            current_code = arduino.get_plastic_code(best_detection['type'])
            
            # Check cooldown
            current_time = time.time()
            if (current_code != last_sent_code or 
                current_time - last_sent_time > send_cooldown):
                
                # Send to Arduino
                arduino.send_plastic_code(
                    best_detection['type'], 
                    best_detection['confidence']
                )
                
                # Update last sent
                last_sent_code = current_code
                last_sent_time = current_time
            
            # Add detection info to frame
            info_text = f"Detected: {best_detection['type']} (Code: {current_code}) [{best_detection['confidence']:.2f}]"
            cv2.putText(annotated_frame, info_text, (10, 30), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        else:
            # No plastic detected
            cv2.putText(annotated_frame, "No plastic detected", (10, 30), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
        
        # Add connection status
        if arduino.ser:
            status_text = f"✅ Connected to Arduino on COM3"
            cv2.putText(annotated_frame, status_text, (10, 60), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
        
        # Display frame
        cv2.imshow("Plastic Segregation System", annotated_frame)
        
        # Break if 'q' pressed
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    
    # Clean up
    cap.release()
    cv2.destroyAllWindows()
    arduino.close()
    print("\n👋 Program terminated")

if __name__ == "__main__":
    main()