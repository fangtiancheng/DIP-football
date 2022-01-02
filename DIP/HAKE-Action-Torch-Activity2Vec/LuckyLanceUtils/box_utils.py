import numpy as np
import os
import pickle
import torch
import cv2
import json


def compute_IOU(rec1, rec2):

    left_column_max = max(rec1[0], rec2[0])
    right_column_min = min(rec1[2], rec2[2])
    up_row_min = min(rec1[3], rec2[3])
    down_row_max = max(rec1[1], rec2[1])

    if left_column_max >= right_column_min or down_row_max >= up_row_min:
        return 0.0

    else:
        S1 = (rec1[2]-rec1[0])*(rec1[3]-rec1[1])
        S2 = (rec2[2]-rec2[0])*(rec2[3]-rec2[1])
        S_cross = (up_row_min-down_row_max) * \
            (right_column_min-left_column_max)
        return S_cross/(S1+S2-S_cross)


def img_preprocess(image, input_size=512):
    image = image[:, :, ::-1]
    image_size = image.shape[:2][::-1]

    resized_image_size = (float(input_size)/max(image_size)
                          * np.array(image_size) // 2 * 2).astype(np.int)
    padding = tuple((input_size-resized_image_size)//2)

    offset = (max(image_size) - np.array(image_size))/2
    offsets = np.array([image_size[1], image_size[0], 0,
                        resized_image_size[0] + padding[1], 0,
                        resized_image_size[1] + padding[0], offset[1],
                        resized_image_size[0], offset[0],
                        resized_image_size[1], max(image_size)], dtype=np.int32)
    offsets = torch.from_numpy(offsets).float()

    return offsets


def convert_kp2d_from_input_to_orgimg(kp2ds, offsets):
    offsets = offsets.float().to(kp2ds.device)
    leftTop = torch.stack([offsets[:, 4] - offsets[:, 8],
                          offsets[:, 2] - offsets[:, 6]], 1)
    kp2ds_org = (kp2ds + 1) * \
        offsets[:, 10].unsqueeze(-1).unsqueeze(-1) / 2 + leftTop.unsqueeze(1)
    return kp2ds_org


def batch_orth_proj(X, camera, keep_dim=False):
    camera = camera.view(-1, 1, 3)
    X_camed = X[:, :, :2] * camera[:, :, 0].unsqueeze(-1)
    X_camed += camera[:, :, 1:]
    if keep_dim:
        X_camed = torch.cat([X_camed, X[:, :, 2].unsqueeze(-1)], -1)
    return X_camed


def vertices_kp3d_projection(j3ds, offsets, cam):
    pj3d = batch_orth_proj(j3ds, cam)
    pj2d = pj3d[:, :, :2]

    pj2d_org = convert_kp2d_from_input_to_orgimg(pj2d, offsets)
    return pj2d_org

#get_MSE()
#input  @point_openpose:   list[[],[],...,[]]
#       @human_dict_romp:  list[[],[],...,[]]
#       @length:           float
#output @the MSE of the given points divided by length


def get_MSE(point_openpose, human_dict_romp, detecetd, length):

    mse_idx = [0, 2, 1, 5, 3, 6, 4, 7, 8, 9, 12, 10, 13, 11, 14]

    detecetd = np.array(detecetd)[mse_idx]

    # print(detecetd)

    point_openpose = np.array(point_openpose).astype(float)
    human_dict_romp = np.array(human_dict_romp).astype(float)

    head_romp = []
    head_romp.append(human_dict_romp[0])

    shoulder_romp = []
    shoulder_romp.append(human_dict_romp[2])
    shoulder_romp.append(human_dict_romp[1])
    shoulder_romp.append(human_dict_romp[5])

    arm_romp = []
    arm_romp.append(human_dict_romp[3])
    arm_romp.append(human_dict_romp[6])
    arm_romp.append(human_dict_romp[4])
    arm_romp.append(human_dict_romp[7])

    hip_romp = []
    hip_romp.append(human_dict_romp[8])
    hip_romp.append(human_dict_romp[9])
    hip_romp.append(human_dict_romp[12])

    leg_romp = []
    leg_romp.append(human_dict_romp[10])
    leg_romp.append(human_dict_romp[13])
    leg_romp.append(human_dict_romp[11])
    leg_romp.append(human_dict_romp[14])

    body_romp = head_romp + shoulder_romp + arm_romp + hip_romp + leg_romp

    head_openpose = []
    head_openpose.append(point_openpose[0])

    shoulder_openpose = []
    shoulder_openpose.append(point_openpose[2])
    shoulder_openpose.append(point_openpose[1])
    shoulder_openpose.append(point_openpose[5])

    arm_openpose = []
    arm_openpose.append(point_openpose[3])
    arm_openpose.append(point_openpose[6])
    arm_openpose.append(point_openpose[4])
    arm_openpose.append(point_openpose[7])

    hip_openpose = []
    hip_openpose.append(point_openpose[8])
    hip_openpose.append(point_openpose[9])
    hip_openpose.append(point_openpose[12])

    leg_openpose = []
    leg_openpose.append(point_openpose[10])
    leg_openpose.append(point_openpose[13])
    leg_openpose.append(point_openpose[11])
    leg_openpose.append(point_openpose[14])

    body_openpose = head_openpose + shoulder_openpose + \
        arm_openpose + hip_openpose + leg_openpose

    MSELoss = []
    x = [(body_romp[i] - body_openpose[i])
         ** 2 for i in range(len(body_openpose))]
    MSELoss = [np.sqrt(np.sum(i)) for i in x]
    MSELoss = np.array(MSELoss) * detecetd

    # print(MSELoss)

    TrueLoss = np.average(MSELoss)

    # normalize
    TrueLoss = TrueLoss / length

    return TrueLoss
