import pickle
import os
import numpy as np
import cv2
import copy
import json
import matplotlib.pyplot as plt
import numpy as np
from sklearn.cluster import KMeans
from tqdm import tqdm
#from sklearn import datasets
from sklearn.datasets import load_iris
from moviepy.editor import *
from box_utils import compute_IOU, cosine_distance
from PIL import Image, ImageDraw, ImageFont
from gensim.models import KeyedVectors
from football_action import football_action

# change the color of the team
mode = 0
# debug_mode
DEBUG = False

RED = (255, 0, 0)
GREEN = (0, 255, 0)
BLUE = (0, 0, 255)
CYAN = (0, 255, 255)
YELLOW = (255, 255, 0)
ORANGE = (255, 165, 0)
PURPLE = (255, 0, 255)
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
PART_COLOR_LIST = [GREEN, CYAN, YELLOW, ORANGE, PURPLE, RED]

BROWN = (128, 42, 42)
JACKIE_BLUE = (11, 23, 70)
YELLOW_BROWN = (240, 230, 140)
SOMECOLOR = (255, 127, 127)
STRAWBERRY = (135, 38, 87)
DARKGREEN = (48, 128, 20)
ID_COLOR_LIST = [DARKGREEN, BROWN, STRAWBERRY, JACKIE_BLUE, BLUE]

skill = football_action['skill']
fight = football_action['fight']

# PaStaNet param
pasta_name_list = np.array([x.strip() for x in open(
    "./PaStaNet-Data/Part_State_93_new.txt").readlines()])
verb_name_list = np.array([x.strip() for x in open(
    "./PaStaNet-Data/verb_list_new.txt").readlines()])

excluded_verbs = [57, 146]
excluded_verb_names = np.delete(verb_name_list, excluded_verbs, axis=0)
topk = 5
SCORE_THRES = 1.5
FONT_SIZE = 18
WHITE = (255, 255, 255)
font = ImageFont.truetype("../tools/inference_tools/consola.ttf", FONT_SIZE)
font_player = ImageFont.truetype(
    "../tools/inference_tools/consola.ttf", 2 * FONT_SIZE)
# possible for use
MAX_HUMAN_NUM = 25
print("Loading w2v data...")
bin_path = "/Disk8/yonglu/Sandwich/Data/GoogleNews-vectors-negative300.bin" 
w2v_model = KeyedVectors.load_word2vec_format(bin_path, binary=True)
print("Loading w2v data finished...")

# demo output filename
video_name = "../DemoVideo/fix_video.mp4"
yolo_file = open("./fix_video.txt")

cap = cv2.VideoCapture(video_name)
speed = cap.get(5)
all_frame_num = cap.get(7)

if DEBUG:
    human_action = {}

human_box_all = {}
for line in yolo_file:
    line = line.rstrip("\n").split(" ")
    frame_idx, human_idx, x_min, y_min, x_delta, y_delta, _, _, _, _, _ = line
    frame_idx, human_idx, x_min, y_min, x_delta, y_delta = int(frame_idx), int(
        human_idx), int(x_min), int(y_min), int(x_delta), int(y_delta)
    if frame_idx not in human_box_all:
        human_box_all[frame_idx] = {}
    if human_idx not in human_box_all[frame_idx]:
        human_box_all[frame_idx][human_idx] = {}
    human_box_all[frame_idx][human_idx]["bbox"] = np.array(
        [x_min, y_min, x_min + x_delta, y_min + y_delta]).astype(int)

# human_box_all
frame_list = []
human_color_all = {}

frame_count = 0
for frame_idx in tqdm(human_box_all):
    success = True
    while success:
        success, frame = cap.read()
        if frame_count == 0:
            blank_image = frame
        x_range = len(frame[0])
        y_range = len(frame)
        frame_count = frame_count + 1
        if frame_count == frame_idx or frame_count > all_frame_num:
            break
    frame_box = human_box_all[frame_idx]
    frame = cv2.GaussianBlur(frame, (3, 3), 1)
    h2v_pkl_path = "./res/" + str(frame_idx) + ".pkl"
    if os.path.exists(h2v_pkl_path) == False:
        continue

    h2v_pkl = pickle.load(open(h2v_pkl_path, "rb"))
    h2v_human_boxes = np.array(h2v_pkl['human_bboxes'])
    h2v_scores = h2v_pkl['human_scores'][:, 0]

    score_filter = h2v_scores > SCORE_THRES

    h2v_human_boxes = h2v_human_boxes[score_filter]
    p_verb = h2v_pkl['p_verb']

    # print(len(h2v_human_boxes))
    human_action_yolo = {}
    for human_key in frame_box:  # the index of yolo

        yolo_box = frame_box[human_key]['bbox']
        x_min, y_min, x_max, y_max = yolo_box
        center_x, center_y = (x_min + x_max) // 2, (y_min + y_max) // 2

        mid_point = [
            [center_y - 1, center_x - 1], [center_y - 1,
                                           center_x], [center_y - 1, center_x + 1],
            [center_y, center_x - 1], [center_y,
                                       center_x], [center_y, center_x + 1],
            [center_y + 1, center_x - 1], [center_y + 1,
                                           center_x], [center_y + 1, center_x + 1]
        ]
        color_list = []
        for coordinate in mid_point:
            color = frame[coordinate[0], coordinate[1]]
            color_list.append(color)

        color_list = np.average(color_list, axis=0)
        # print(color_list)

        if human_key not in human_color_all:
            human_color_all[human_key] = []
        human_color_all[human_key].append(color_list)

        IOU_list = []
        # get the biggest one, (if bigger than 0.5)
        for h2v_human_idx in range(len(h2v_human_boxes)):
            h2v_human_box = h2v_human_boxes[h2v_human_idx]
            # seach h2v for each yolo
            IOU_list.append(compute_IOU(yolo_box, h2v_human_box))

        h2v_human_idx = np.argmax(IOU_list)
        # if  IOU_list[h2v_human_idx] > 0.5: # optional
        verb_scores = p_verb[h2v_human_idx]
        # Get verb and pasta names to draw.
        verb_scores = np.delete(verb_scores, excluded_verbs, axis=0)
        verb_top_idxs = np.argsort(verb_scores)[::-1]
        verb_draw_names = []
        for top_idx, verb_name in enumerate(excluded_verb_names[verb_top_idxs]):
            verb_idx = verb_top_idxs[top_idx]
            if verb_name not in verb_draw_names:
                verb_draw_names.append(verb_name)
            if len(verb_draw_names) == topk:
                break
        human_action_yolo[human_key] = verb_draw_names

    if DEBUG:
        human_action[frame_idx] = human_action_yolo

color_pair = []
for human_idx in human_color_all:
    avg_color = np.average(human_color_all[human_idx], axis=0)
    human_color_all[human_idx] = avg_color
    color_pair.append(avg_color)
# print(human_color_all)

# cluster the teams
color_pair = np.array(color_pair)
estimator = KMeans(n_clusters=2)
estimator.fit(color_pair)
label_pred = estimator.labels_
x0 = color_pair[label_pred == 0]
x1 = color_pair[label_pred == 1]

color_mode0_idx = []
color_mode1_idx = []
for color in x0:
    for human_key in human_color_all:
        if np.sum(np.square(color - human_color_all[human_key])) < 0.5:
            color_mode0_idx.append(human_key)

for color in x1:
    for human_key in human_color_all:
        if np.sum(np.square(color - human_color_all[human_key])) < 0.5:
            color_mode1_idx.append(human_key)

print(color_mode0_idx, ". These players are in one team.")
print(color_mode1_idx, ". These players are in one team.")

print("Begin to draw")
if mode == 0:
    color_mode_idx = color_mode0_idx
elif mode == 1:
    color_mode_idx = color_mode1_idx

MAX_HUMAN_NUM = len(color_mode_idx)
# human_box_all
frame_list = []
blank_image = np.zeros_like(blank_image)
for frame_idx in tqdm(human_box_all):
    frame_box = human_box_all[frame_idx]
    h2v_pkl_path = "./res/" + str(frame_idx) + ".pkl"
    if os.path.exists(h2v_pkl_path) == False:
        continue
    image = cv2.cvtColor(blank_image, cv2.COLOR_BGR2RGB)
    im_shape = list(image.shape)

    # Construct a black sidebar.
    ones_shape = copy.deepcopy(im_shape)
    # a broader side
    ones_shape[1] = 150 * (MAX_HUMAN_NUM // 4 + 1)
    image_ones = np.ones(ones_shape, dtype=image.dtype) * 0
    image = np.concatenate((image, image_ones), axis=1)

    pil_image = Image.fromarray(image).convert('RGBA')

    h2v_pkl = pickle.load(open(h2v_pkl_path, "rb"))
    h2v_human_boxes = np.array(h2v_pkl['human_bboxes'])
    h2v_scores = h2v_pkl['human_scores'][:, 0]

    score_filter = h2v_scores > SCORE_THRES

    h2v_human_boxes = h2v_human_boxes[score_filter]
    p_verb = h2v_pkl['p_verb']

    # White rectangle as bottom.
    overlay = Image.new('RGBA', pil_image.size, WHITE+(0,))
    overlay_draw = ImageDraw.Draw(overlay)

    canvas = Image.new('RGBA', pil_image.size, WHITE+(0,))
    draw = ImageDraw.Draw(canvas)

    extra_offset = FONT_SIZE
    # print(len(h2v_human_boxes))
    human_action_yolo = {}
    human_count = 0
    for human_key in frame_box:  # the index of yolo
        if human_key not in color_mode_idx:
            continue
        yolo_box = frame_box[human_key]['bbox']
        x_min, y_min, x_max, y_max = yolo_box
        center_x, center_y = (x_min + x_max) // 2, (y_min + y_max) // 2
        IOU_list = []
        # get the biggest one, (if bigger than 0.5)
        for h2v_human_idx in range(len(h2v_human_boxes)):
            h2v_human_box = h2v_human_boxes[h2v_human_idx]
            # seach h2v for each yolo
            IOU_list.append(compute_IOU(yolo_box, h2v_human_box))

        h2v_human_idx = np.argmax(IOU_list)
        # if  IOU_list[h2v_human_idx] > 0.5: # optional
        verb_scores = p_verb[h2v_human_idx]
        # Get verb and pasta names to draw.
        verb_scores = np.delete(verb_scores, excluded_verbs, axis=0)
        verb_top_idxs = np.argsort(verb_scores)[::-1]
        verb_draw_names = []
        for top_idx, verb_name in enumerate(excluded_verb_names[verb_top_idxs]):
            verb_idx = verb_top_idxs[top_idx]
            if verb_name not in verb_draw_names:
                verb_draw_names.append(verb_name)
            if len(verb_draw_names) == topk:
                break
        human_action_yolo[human_key] = verb_draw_names

        # get mapping Google distance
        for new_action in verb_draw_names:
            cos_score = []
            action_vector = []
            for action in new_action:
                if action not in w2v_model:
                    continue
                action_vector.append(w2v_model[action])
            action_vector = np.vstack(action_vector)
            # vector = np.sum(vector,axis=0)
            action_vector = np.mean(action_vector,axis=0)
            # get skill
            for action in skill:
                action_vect_map = []
                # w2v_model
                action = action.replace("_", ' ')
                action = action.replace("-", ' ')
                action = action.replace(".", ' ')
                action = action.replace(",", ' ')
                action_list = action.split(" ")
                for action_clip in action_list:
                    if action_clip not in w2v_model:
                        continue
                    action_vect_map.append(w2v_model[action_clip])
                if len(action_vect_map) > 0:
                    action_vect_map = np.vstack(action_vect_map)
                    action_vect_map = np.mean(action_vect_map,axis=0)
                    cos_score.append(cosine_distance(action_vector, action_vect_map))
                else:
                    cos_score.append(0.)
            skill_idx = np.argmax(cos_score)
            skill_name = skill[skill_idx]

            # get fight
            cos_score = []
            for action in fight:
                action_vect_map = []
                # w2v_model
                action = action.replace("_", ' ')
                action = action.replace("-", ' ')
                action = action.replace(".", ' ')
                action = action.replace(",", ' ')
                action_list = action.split(" ")
                for action_clip in action_list:
                    if action_clip not in w2v_model:
                        continue
                    action_vect_map.append(w2v_model[action_clip])
                if len(action_vect_map) > 0:
                    action_vect_map = np.vstack(action_vect_map)
                    action_vect_map = np.mean(action_vect_map,axis=0)
                    cos_score.append(cosine_distance(action_vector, action_vect_map))
                else:
                    cos_score.append(0.)
            fight_idx = np.argmax(cos_score)
            fight_name = fight[fight_idx]

        # draw one frame
        x_space = center_x - 19
        y_space = center_y + 19
        if human_key >= 10:
            fontsize = 1
            y_space = y_space - 3
        # cv2.circle(
        #     pil_image, (center_x, center_y), 30, (255, 255, 255), 2)
        draw.text((x_space, y_space), str(human_key),
                  font=font_player, fill=CYAN+(255, ))

        # Update sidebar. @ XudongLu dynamic bar width
        x_axis = x_range + 1 + (human_count // 4) * 150
        if human_count % 4 == 0:
            extra_offset = 0

        draw.text((x_axis, 3+extra_offset), 'ID: '+str(human_key), font=font,
                  fill=CYAN+(255, ))  # +' {:.3f}'.format(verb_scores[verb_idx])

        extra_offset += FONT_SIZE + 2
        for draw_name in verb_draw_names:
            # x_axis = im_shape[1]+1
            # +' {:.3f}'.format(verb_scores[verb_idx])
            draw.text((x_axis, 3+extra_offset), draw_name,
                      font=font, fill=GREEN+(255, ))
            extra_offset += FONT_SIZE
        draw.text((x_axis, 3+extra_offset), skill_name,
                    font=font, fill=CYAN+(255, ))
        extra_offset += FONT_SIZE

        draw.text((x_axis, 3+extra_offset), fight_name,
                    font=font, fill=CYAN+(255, ))
        extra_offset += FONT_SIZE

        draw.text((x_axis, 3+extra_offset), '────────',
                  font=font, fill=CYAN+(255, ))
        extra_offset += FONT_SIZE
        human_count = human_count + 1

    # Combine image and canvas
    pil_image = Image.alpha_composite(pil_image, overlay)
    pil_image = Image.alpha_composite(pil_image, canvas)
    pil_image = pil_image.convert('RGB')
    cv2_image = cv2.cvtColor(np.array(pil_image), cv2.COLOR_RGB2BGR)
    frame_list.append(cv2_image)

new_clip = ImageSequenceClip(frame_list, fps=(int)(speed))
vis_path = "test_out.mp4"
new_clip.write_videofile(vis_path)
print(len(frame_list))
print("Team identification finished")

if DEBUG:
    with open("human_action.json", "w") as file:
        json.dump(human_action, file)
