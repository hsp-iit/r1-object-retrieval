from ultralytics import YOLO

model = YOLO("yolov8s.pt");

results = model.predict(source="0", show=True)

print(results)