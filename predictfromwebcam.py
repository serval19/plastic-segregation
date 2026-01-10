import cv2
from ultralytics import YOLO

# 1. Load your trained YOLO11 plastic detection model
# Replace 'best.pt' with the actual path to your trained weight file
model = YOLO('./trainresults/weights/best.pt') 

# 2. Initialize the webcam (0 is usually the default camera)
cap = cv2.VideoCapture(0)

if not cap.isOpened():
    print("Error: Could not open webcam.")
    exit()

print("Webcam started. Press 'q' to exit.")

while True:
    # Capture frame-by-frame
    success, frame = cap.read()
    
    if not success:
        break

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