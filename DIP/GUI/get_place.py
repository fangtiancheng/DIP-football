# https://blog.csdn.net/hxxjxw/article/details/109269306

import cv2
import os
import pickle
import numpy as np
pwd = "/media/ftc/H/DIP/GUI"
# for root,_, filenames in os.walk(pwd + "/out"):
#     for filename in filenames:
#         os.remove(os.path.join(root, filename))


def on_EVENT_LBUTTONDOWN(event, x, y, flags, param):
    if event == cv2.EVENT_LBUTTONDOWN:
        xy = "%d,%d" % (x, y)
        a.append(x)
        b.append(y)
        cv2.circle(img, (x, y), 1, (0, 0, 255), thickness=-1)
        cv2.putText(img, xy, (x, y), cv2.FONT_HERSHEY_PLAIN,
                    1.0, (0, 0, 255), thickness=2)
        cv2.imshow("image", img)


def regularization(filename, mode, src, dst=None):
    img = cv2.imread(filename)
    img = cv2.cvtColor(img, cv2.COLOR_BGRA2RGB)
    h_s, w_s, _ = img.shape
    src = np.array(src).astype(np.float32)

    template = cv2.imread(pwd + "/world_cup_template.png")
    template = cv2.cvtColor(template, cv2.COLOR_BGRA2RGB)

    if mode == 0:
        # left gate
        h_t, w_t, _ = template.shape
        dst = np.array([
            [7 * w_s / w_t, 137 * h_s / h_t],
            [7 * w_s / w_t, 541 * h_s / h_t],
            [165 * w_s / w_t, 541 * h_s / h_t],
            [165 * w_s / w_t, 137 * h_s / h_t]
        ], dtype=np.float32)

        M = cv2.getPerspectiveTransform(src, dst)
        new_img = cv2.warpPerspective(img, M, (w_s, h_s))

        cv2.imwrite("left_gate_" + filename.split("/")
                    [-1][:-4] + ".jpg", new_img)

    elif mode == 1:
        # center circle
        h_t, w_t, _ = template.shape
        dst = np.array([
            [525 * w_s / w_t, 249 * h_s / h_t],
            [433 * w_s / w_t, 340 * h_s / h_t],
            [525 * w_s / w_t, 430 * h_s / h_t],
            [615 * w_s / w_t, 340 * h_s / h_t]
        ], dtype=np.float32)

        M = cv2.getPerspectiveTransform(src, dst)
        new_img = cv2.warpPerspective(img, M, (w_s, h_s))

        cv2.imwrite(pwd + "/center_" + filename.split("/")[-1][:-4] + ".jpg", new_img)

    elif mode == 2:
        # right gate
        h_t, w_t, _ = template.shape
        dst = np.array([
            [885 * w_s / w_t, 139 * h_s / h_t],
            [885 * w_s / w_t, 540 * h_s / h_t],
            [1043 * w_s / w_t, 540 * h_s / h_t],
            [1043 * w_s / w_t, 139 * h_s / h_t]
        ], dtype=np.float32)

        M = cv2.getPerspectiveTransform(src, dst)
        new_img = cv2.warpPerspective(img, M, (w_s, h_s))

        cv2.imwrite(pwd + "/right_gate_" + filename.split("/")
                    [-1][:-4] + ".jpg", new_img)

    elif mode == 3:
        # customize
        h_t, w_t, _ = template.shape
        dst = np.array([
            [dst[0][0] * w_s / w_t, dst[0][1] * h_s / h_t],
            [dst[1][0] * w_s / w_t, dst[1][1] * h_s / h_t],
            [dst[2][0] * w_s / w_t, dst[2][1] * h_s / h_t],
            [dst[3][0] * w_s / w_t, dst[3][1] * h_s / h_t]
        ], dtype=np.float32)
        M = cv2.getPerspectiveTransform(src, dst)
        new_img = cv2.warpPerspective(img, M, (w_s, h_s))

        cv2.imwrite(pwd + "/out/customize_" + filename.split("/")
                    [-1][:-4] + ".jpg", new_img)

    else:
        print("No such Mode")
        return

    return M


img = cv2.imread(pwd + '/test_frame.jpg')
a = []
b = []

cv2.namedWindow("image")
cv2.setMouseCallback("image", on_EVENT_LBUTTONDOWN)
cv2.imshow("image", img)
cv2.waitKey(0)

try:

    src = np.array([
        [a[0], b[0]],
        [a[1], b[1]],
        [a[2], b[2]],
        [a[3], b[3]]
    ], dtype=np.float32)

    print(src)

except:
    print("The number of node choosen is not 4, Try again")

assert len(a) == 4
assert len(b) == 4

img = cv2.imread(pwd + '/world_cup_template.png')
a = []
b = []

cv2.namedWindow("image")
cv2.setMouseCallback("image", on_EVENT_LBUTTONDOWN)
cv2.imshow("image", img)
cv2.waitKey(0)

try:

    dst = np.array([
        [a[0], b[0]],
        [a[1], b[1]],
        [a[2], b[2]],
        [a[3], b[3]]
    ], dtype=np.float32)

    print(dst)

except:
    print("The number of node choosen is not 4, Try again")

assert len(a) == 4
assert len(b) == 4

M = regularization(pwd + "/test_frame.jpg", 3, src, dst)
print(M)

param = {}
param['src'] = src
param['dst'] = dst
param['M'] = M

with open(pwd + "/out/human_trans.pkl", "wb") as file:
    pickle.dump(param, file)

exit(0)