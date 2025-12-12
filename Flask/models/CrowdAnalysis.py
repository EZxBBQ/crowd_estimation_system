from ultralytics import YOLO
import numpy
import os

img_height = 736
img_width = 1280
section_amount = 4

model_dir = os.path.dirname(os.path.abspath(__file__))

def RunYolo(img):
    detectionModel = YOLO(os.path.join(model_dir, "yolov8s.pt"))
    poseModel = YOLO(os.path.join(model_dir, "yolov8s-pose.pt"))

    # 1280x736 input resolution, keep people at confidential value at 10% and no overlapping less than 70%
    detectionResults = detectionModel(img, save=False, imgsz=(img_height, img_width), conf=0.1, iou=0.7) 
    poseResults = poseModel(img, save=False, imgsz=(img_height, img_width), conf=0.4, iou=0.7, classes=[0])
    level, standing, sitting, peopleCount = AnalyzeResults(detectionResults, poseResults)
    return level, standing, sitting, peopleCount



# get crowd density information from the image 
def AnalyzeResults(detectionResults, poseResults):
    # separate image into sections
    sectionHeight = img_height // section_amount
    sectionWidth = img_width // section_amount
    sections = [[0 for _ in range(section_amount)] for _ in range(section_amount)]

    # find centroid of each detected person
    centroids = []
    coordinates = detectionResults[0].boxes.xyxy.cpu().numpy() 
    classes = detectionResults[0].boxes.cls.cpu().numpy()
    for box, cls_id in zip(coordinates, classes):
        if int(cls_id) == 0:  # class 0 is person
            x1, y1, x2, y2 = box 
            cx = (x1 + x2) / 2 
            cy = (y1 + y2) / 2 
            centroids.append((cx, cy))
    
    peopleCount = len(centroids)
    
    for cx, cy in centroids:
        row = int(cy // sectionHeight)
        col = int(cx // sectionWidth)
        row = min(row, section_amount - 1)
        col = min(col, section_amount - 1)

        sections[row][col] += 1

    # classify posture of each detected person
    standing = 0
    sitting = 0
    if poseResults[0].keypoints is not None:
        data = poseResults[0].keypoints.data.cpu().numpy()
        for kp in data:
            h_l = kp[11]
            h_r = kp[12]
            k_l = kp[13]
            k_r = kp[14]
            a_l = kp[15]
            a_r = kp[16]

            if min(h_l[2], h_r[2], k_l[2], k_r[2], a_l[2], a_r[2]) < 0.4:
                sitting += 1
                continue

            hip_y = (h_l[1] + h_r[1]) / 2
            knee_y = (k_l[1] + k_r[1]) / 2
            ankle_y = (a_l[1] + a_r[1]) / 2

            leg = ankle_y - hip_y
            thigh = knee_y - hip_y

            if leg <= 0:
                sitting += 1
                continue

            leg_norm = leg / img_height
            ratio = thigh / leg

            if leg_norm > 0.2 and ratio >= 0.6:
                standing += 1
            else:
                sitting += 1


    level = LevelEvaluation(sections, standing, sitting)
    print(level)
    return level, standing, sitting, peopleCount



# evaluate crowd density level based on section data and posture information
def LevelEvaluation(sections, standing, sitting):
    total = 0
    peak = 0
    for r in range(section_amount):
        for c in range(section_amount):
            v = sections[r][c]
            total += v
            if v > peak:
                peak = v

    if total == 0:
        return "LOW"

    standingRatio = standing / max(standing + sitting, 1)
    score = (0.6 * peak + 0.4 * (total / 4)) * (1.0 + 0.5 * standingRatio)

    if score < 2:
        return "LOW"
    elif score < 5:
        return "MEDIUM"
    elif score < 8:
        return "DENSE"
    else:
        return "FULL"

