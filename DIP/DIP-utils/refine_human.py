import pickle
import os
from gensim.utils import trim_vocab_by_freq
import numpy as np
import cv2
import copy
import json
import torch
import pickle
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

# # change the color of the team
# mode = 0
# debug_mode
DEBUG = True
##########################################################################################

# demo output filename, Modify these codes
video_name = "../inputfile/demo.mp4"
yolo_file = open("../output/demo.txt")
M = np.array(pickle.load(open("../trans_out/human_trans.pkl", "rb"))['M']).astype(np.float32)
use_network_sportsfield = False
network_field_path = "../trans_out/demo_field_out.pkl"

##########################################################################################

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
font = ImageFont.truetype("../HAKE-Action-Torch-Activity2Vec/tools/inference_tools/consola.ttf", FONT_SIZE)
font_player = ImageFont.truetype(
    "../HAKE-Action-Torch-Activity2Vec/tools/inference_tools/consola.ttf", 2 * FONT_SIZE)
# possible for use
MAX_HUMAN_NUM = 25
print("Loading w2v data...")
bin_path = "../GoogleNews-vectors-negative300.bin"
w2v_model = KeyedVectors.load_word2vec_format(bin_path, binary=True)
print("Loading w2v data finished...")

cap = cv2.VideoCapture(video_name)
speed = cap.get(5)
all_frame_num = cap.get(7)

if DEBUG:
    human_action = {}

template = cv2.imread("./world_cup_template.png")

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
    h2v_pkl_path = "../output/res/" + str(frame_idx) + ".pkl"
    if os.path.exists(h2v_pkl_path) == False:
        continue

    h2v_pkl = pickle.load(open(h2v_pkl_path, "rb"))
    h2v_human_boxes = np.array(h2v_pkl['human_bboxes'])
    h2v_scores = h2v_pkl['human_scores'][:, 0]

    score_filter = h2v_scores > SCORE_THRES

    h2v_human_boxes = h2v_human_boxes[score_filter]
    p_verb = h2v_pkl['p_verb']

    # print(len(h2v_human_boxes))
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
for mode in range(2):
    human_action[mode] = {}
    if mode == 0:
        color_mode_idx = color_mode0_idx
    elif mode == 1:
        color_mode_idx = color_mode1_idx

    MAX_HUMAN_NUM = len(color_mode_idx)
    # human_box_all
    frame_list = []
    blank_image = np.zeros_like(blank_image)
    for frame_idx in tqdm(human_box_all):
        human_action[mode][frame_idx] = {}
        frame_box = human_box_all[frame_idx]
        h2v_pkl_path = "../output/res/" + str(frame_idx) + ".pkl"
        if os.path.exists(h2v_pkl_path) == False:
            continue

        template = cv2.cvtColor(template, cv2.COLOR_BGR2RGB)
        h_b, w_b, _ = blank_image.shape
        image = cv2.resize(template, (w_b, h_b))
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
        overlay = pil_image
        overlay_draw = ImageDraw.Draw(overlay)

        canvas = Image.new('RGBA', pil_image.size, WHITE+(0,))
        draw = ImageDraw.Draw(canvas)

        extra_offset = FONT_SIZE
        # print(len(h2v_human_boxes))
        human_count = 0
        for human_key in frame_box:  # the index of yolo
            if human_key not in color_mode_idx:
                continue
            human_action[mode][frame_idx][human_key] = {}
            yolo_box = frame_box[human_key]['bbox']
            x_min, y_min, x_max, y_max = yolo_box
            center_x, center_y = (x_min + x_max) // 2, int(y_max)
            IOU_list = []
            # get the biggest one, (if bigger than 0.5)
            for h2v_human_idx in range(len(h2v_human_boxes)):
                h2v_human_box = h2v_human_boxes[h2v_human_idx]
                # seach h2v for each yolo
                IOU_list.append(compute_IOU(yolo_box, h2v_human_box))

            h2v_human_idx = np.argmax(IOU_list)
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
            human_action[mode][frame_idx][human_key]['verb'] = verb_draw_names

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
                action_vector = np.mean(action_vector, axis=0)
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
                        action_vect_map = np.mean(action_vect_map, axis=0)
                        cos_score.append(cosine_distance(
                            action_vector, action_vect_map))
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
                        action_vect_map = np.mean(action_vect_map, axis=0)
                        cos_score.append(cosine_distance(
                            action_vector, action_vect_map))
                    else:
                        cos_score.append(0.)
                fight_idx = np.argmax(cos_score)
                fight_name = fight[fight_idx]

            human_action[mode][frame_idx][human_key]['skill'] = skill_name
            human_action[mode][frame_idx][human_key]['fight'] = fight_name


            point = np.array([center_x, center_y, 1])
            # TODO: Draw a person
            # using human mapping sports field
            if use_network_sportsfield == False:
                new_space = np.dot(M, point)
                new_space = new_space / new_space[2]
            # using network mapping sports field
            else:
                net_field = pickle.load(open(network_field_path, "rb"))
                warped_frm, optim_homography = net_field
                x = torch.tensor(point[0] / w_b - 0.5).float()
                y = torch.tensor(point[1] / h_b - 0.5).float()
                xy = torch.stack([x, y, torch.ones_like(x)])
                xy_warped = torch.matmul(optim_homography.cpu(), xy)  # H.bmm(xy)
                xy_warped, z_warped = xy_warped.split(2, dim=1)

                # we multiply by 2, since our homographies map to
                # coordinates in the range [-0.5, 0.5] (the ones in our GT datasets)
                xy_warped = 2.0 * xy_warped / (z_warped + 1e-8)
                x_warped, y_warped = torch.unbind(xy_warped, dim=1)
                # [-1, 1] -> [0, 1]
                x_warped = (x_warped.item() * 0.5 + 0.5) * w_b
                y_warped = (y_warped.item() * 0.5 + 0.5) * h_b
                new_space = [(int)(x_warped), (int)(y_warped)]
            # print(point)
            # print(new_space)
            draw.text((new_space[0], new_space[1]), str(human_key),
                    font=font_player, fill=CYAN+(255, ))

            # Update sidebar. @TODO:  dynamic bar width
            x_axis = x_range + 1 + (human_count // 4) * 150
            if human_count % 4 == 0:
                extra_offset = 0

            draw.text((x_axis, 3+extra_offset), 'ID: '+str(human_key), font=font,
                    fill=CYAN+(255, ))  # +' {:.3f}'.format(verb_scores[verb_idx])

            extra_offset += FONT_SIZE + 2
            for draw_name in verb_draw_names:
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
    vis_path = "../output/test_out_" + str(mode) +".mp4"
    new_clip.write_videofile(vis_path)
    print(len(frame_list))
    print("Team identification finished")

    if DEBUG:
        with open("../output/json/test_out_" + str(mode) + ".json", "w") as file:
            json.dump(human_action[mode], file)
