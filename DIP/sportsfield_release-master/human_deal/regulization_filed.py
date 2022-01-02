import matplotlib.pyplot as plt
import numpy as np
import cv2
import pickle


def show(img):
    if img.ndim == 2:
        plt.imshow(img, cmap='gray')
    else:
        plt.imshow(cv2.cvtColor(img, cv2.COLOR_BGR2RGB))
    plt.show()


def regularization(filename, mode, src, dst=None):
    img = cv2.imread(filename)
    img = cv2.cvtColor(img, cv2.COLOR_BGRA2RGB)
    h_s, w_s, _ = img.shape
    src = np.array(src).astype(np.float32)

    template = cv2.imread("../data/world_cup_template.png")
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

        cv2.imwrite("center_" + filename.split("/")[-1][:-4] + ".jpg", new_img)

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

        cv2.imwrite("right_gate_" + filename.split("/")
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

        cv2.imwrite("customize_" + filename.split("/")
                    [-1][:-4] + ".jpg", new_img)

    else:
        print("No such Mode")
        return

    return M


if __name__ == "__main__":
    print("Regularization the sports field: Anti-clockwize")

    src = np.array([
        [640, 110],
        [873, 778],
        [139, 165],
        [1139, 116]
    ], dtype=np.float32)

    dst = np.array([
        [525, 4],
        [525, 676],
        [163, 140],
        [887, 140]
    ], dtype=np.float32)

    M = regularization("../../trans_out/test_frame.jpg", 3, src, dst).astype(np.float32)
    print("The transformation Matrix is: ")
    print(M)
    with open("../../trans_out/human_trans.pkl", "wb") as file:
        pickle.dump(M, file)
