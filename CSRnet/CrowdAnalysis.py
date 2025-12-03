from lwcc import LWCC
from ultralytics import YOLO
import cv2

# CSRnet implementation (bad for few people, good for crowd)
def run_csrnet(img):
    count = LWCC.get_count(img, model_name = "CSRNet", model_weights="SHA")
    print("Predicted Count:", count)
    return int(count)

# YOLOv8 implementation (bad for crowd, good for few people)
def run_yolo(img):
    model = YOLO("yolov8m.pt")  # medium model
    results = model(img, imgsz=960) # 960 input resolution
    person_count = 0

    for result in results:
        if result.boxes is not None:
            for cls in result.boxes.cls:
                if int(cls) == 0:   # class 0 = person in COCO
                    person_count += 1

    print("Detected People:", person_count)
    return person_count

def choose_model(img):
    model = YOLO("yolov8m.pt")
    results = model(img, imgsz=640)

    img_data = cv2.imread(img)
    H, W = img_data.shape[:2]
    img_area = H * W

    total_area = 0.0
    yolo_count = 0

    for result in results:
        if result.boxes is not None:
            boxes = result.boxes.xyxy.cpu().numpy()
            classes = result.boxes.cls.cpu().numpy()
            confs = result.boxes.conf.cpu().numpy()

            for box, cls, conf in zip(boxes, classes, confs):
                if int(cls) == 0 and conf > 0.5:
                    x1, y1, x2, y2 = box

                    x1 = max(0, min(W - 1, x1))
                    x2 = max(0, min(W - 1, x2))
                    y1 = max(0, min(H - 1, y1))
                    y2 = max(0, min(H - 1, y2))

                    area = max(0.0, (x2 - x1) * (y2 - y1))
                    total_area += area
                    yolo_count += 1

    if yolo_count == 0:
        return run_csrnet(img)

    mean_box_ratio = (total_area / yolo_count) / img_area
    print("mean_box_ratio:", mean_box_ratio)

    if mean_box_ratio > 0.015:
        return run_yolo(img)
    else:
        return run_csrnet(img)

choose_model("0239.jpg")
