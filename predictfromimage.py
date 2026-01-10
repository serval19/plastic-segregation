from ultralytics import YOLO
import os

# Then your detection code
model = YOLO("./trainresults/weights/best.pt")
test_image_path = "./testimage/22.jpg"
results = model.predict(source=test_image_path, save=True)