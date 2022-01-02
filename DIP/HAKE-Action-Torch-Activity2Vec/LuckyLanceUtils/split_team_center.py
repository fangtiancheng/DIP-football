#####################################################################################
#  Author:    Xudong Lu                                                             #
#  E-mail:    luxudong2001@sjtu.edu.cn                                              #
#  HomePage:  https://github.com/Lucky-Lance                                        #
#  Inside frame: Get pairwise IOU and omit if IOU > 0.5                             #
#  Between frame: Get temporal IOU and omit if IOU < 0.15                           #
#  Make use of the human score DEMO.SCORE_THRES = 1.5                               #
#####################################################################################

import pickle
import os
import numpy as np
import cv2
import matplotlib.pyplot as plt
import numpy as np
from sklearn.cluster import KMeans
from tqdm import tqdm
#from sklearn import datasets
from sklearn.datasets import load_iris
from moviepy.editor import *

from box_utils import compute_IOU

# params need to be changed
# mode: 0 or 1, for different colors
color_mode = 1
# insider mask: needed
insider_ratio = 0.6
# relavent mask: little use
relavent_ratio = 0.6
# param of PaStaNet
SCORE_THRES = 1.5

success = True
x_axis, y_axis = 0, 0
count = 0
img_list = []

cap = cv2.VideoCapture("../DemoVideo/fix_video.mp4")
total_frame = cap.get(7)
speed = cap.get(5)
print(total_frame, speed)

while success and count < total_frame:
    success, frame = cap.read()
    pickle_path = "../out/res/" + str(count) + ".pkl"
    if os.path.exists(pickle_path) == False:
        success = False
    outeample = pickle.load(open(pickle_path, "rb"))

    # GaussianBlur to Blur the image
    frame = cv2.GaussianBlur(frame, (3, 3), 1)
    # Get prev result
    outeample_numpy = np.array(outeample['human_bboxes'])
    human_score = np.array(outeample['human_scores'])

    # Get insider IOU
    h_num = len(outeample_numpy)
    mask = np.ones(h_num).astype(int)
    for i in range(h_num):
        if mask[i] == 0:
            continue
        # bounded by score
        if human_score[i] < SCORE_THRES:
            mask[i] = 0
            continue
        for j in range(i + 1, h_num):
            if compute_IOU(outeample_numpy[i], outeample_numpy[j]) > insider_ratio:
                mask[j] = 0

    # Get frameWise IOU, from the second frame
    if count != 0 and relavent_ratio != 0:
        h_num_old = len(outeample_numpy_old)
        for i in range(h_num):
            max_iou = 0
            if mask[i] == 0:
                continue
            for old_i in range(h_num_old):
                if mask_old[old_i] == 0:
                    continue
                iou_pre = compute_IOU(
                    outeample_numpy[i], outeample_numpy_old[old_i])
                max_iou = max(max_iou, iou_pre)
            if max_iou < relavent_ratio and max_iou > 1e-3:
                mask[i] = 0

    center_slide = []
    for per_human_box in outeample_numpy:
        x_min, y_min, x_max, y_max = per_human_box
        center_x = (int)(x_min + x_max) // 2
        center_y = (int)(y_min + y_max) // 2
        mid_point = [
            [center_y - 1, center_x - 1], [center_y - 1,
                                           center_x], [center_y - 1, center_x + 1],
            [center_y, center_x - 1], [center_y,
                                       center_x], [center_y, center_x + 1],
            [center_y + 1, center_x - 1], [center_y + 1,
                                           center_x], [center_y + 1, center_x + 1]
        ]
        center_slide.append(mid_point)
    color_pair = []
    for mid_point in center_slide:
        color_list = []
        for coordinate in mid_point:
            color = frame[coordinate[0], coordinate[1]]
            color_list.append(color)
        color_list = np.average(color_list, axis=0)
        color_pair.append(color_list)
    for idx in range(len(color_pair)):
        color_pair[idx] = color_pair[idx].tolist()
    color_pair = np.array(color_pair)
    estimator = KMeans(n_clusters=2)
    estimator.fit(color_pair)
    label_pred = estimator.labels_
    x0 = color_pair[label_pred == 0]
    x1 = color_pair[label_pred == 1]
    x0_avg = np.average(x0)
    x1_avg = np.average(x1)
    if x0_avg < x1_avg:
        result = [x0, x1]
    else:
        result = [x1, x0]
    cluster = []
    for i in range(len(result)):
        cluster_per = []
        for result_per in result[i]:
            for idx in range(len(color_pair)):
                if np.sum(np.abs(result_per - color_pair[idx])) < 0.5:
                    cluster_per.append(idx)
        cluster.append(cluster_per)
    img = np.zeros((len(frame), len(frame[0])))
    for idx in cluster[color_mode]:
        if mask[idx] == 0:
            continue
        fontsize = 2
        x_space = center_slide[idx][4][1]-19
        y_space = center_slide[idx][4][0]+19
        if idx >= 10:
            fontsize = 1
            y_space = y_space - 3
        cv2.circle(
            img, (center_slide[idx][4][1], center_slide[idx][4][0]), 30, (255, 255, 255), 2)
        cv2.putText(img, str(idx), (x_space, y_space),
                    cv2.FONT_HERSHEY_SIMPLEX, fontsize, (255, 255, 255), 2)
    img = np.dstack(3 * [img]).astype('uint8')
    img_list.append(img)

    mask_old = mask
    outeample_numpy_old = outeample_numpy
    count = count + 1

new_clip = ImageSequenceClip(img_list, fps=(int)(speed))
vis_path = "test_out.mp4"
new_clip.write_videofile(vis_path)
