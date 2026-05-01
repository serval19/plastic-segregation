import cv2
from ultralytics import YOLO

# 1. Load your trained YOLO11 plastic detection model
# Replace 'best.pt' with the actual path to your trained weight file
model = YOLO('./15thapril-model-train-results/weights/best.pt')  # Original weight path is :  ./trainresults/weights/best.pt

# 2. Initialize the webcam - MODIFIED FOR EXTERNAL WEBCAM
print("📷 Looking for external webcam...")

# Try different indices to find your external webcam
# Usually: 0 = built-in webcam, 1 = first external webcam, 2 = second external webcam, etc.
camera_index = 1  # Start with 1 (first external camera)

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

# If external webcam not found, try built-in camera as fallback
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

print(f"📷 Webcam started. Press 'q' to exit.")
print(f"🎯 Camera resolution: {int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))}x{int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))}")

while True:
    # Capture frame-by-frame
    success, frame = cap.read()
    
    if not success:
        print("⚠️ Failed to grab frame, retrying...")
        continue

    # 3. Run YOLO11 inference on the frame
    # stream=True is more memory efficient for real-time video
    results = model.predict(source=frame, conf=0.5, show=False, stream=True)

    # 4. Process and visualize the results
    for r in results:
        # This uses the built-in plot() method to draw boxes/labels
        annotated_frame = r.plot()

        # Display the resulting frame
        cv2.imshow("YOLO11 Plastic Type Detection", annotated_frame)

    # 5. Break the loop if 'q' is pressed
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Clean up
cap.release()
cv2.destroyAllWindows()
print("👋 Program terminated")