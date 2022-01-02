import os

import numpy as np
import torch
import imageio
import matplotlib.pyplot as plt
from PIL import Image
from tqdm import tqdm_notebook as tqdm

from utils import utils, warp, image_utils, constant_var
from models import end_2_end_optimization
from options import fake_options
import cv2

# if want to run on CPU, please make it False
constant_var.USE_CUDA = False
utils.fix_randomness()

# if GPU is RTX 20XX, please disable cudnn
torch.backends.cudnn.enabled = True

# set some options
opt = fake_options.FakeOptions()
opt.batch_size = 1
opt.coord_conv_template = True
opt.error_model = 'loss_surface'
opt.error_target = 'iou_whole'
opt.goal_image_path = './data/demo_test.png'
opt.guess_model = 'init_guess'
opt.homo_param_method = 'deep_homography'
opt.load_weights_error_model = 'pretrained_loss_surface'
opt.load_weights_upstream = 'pretrained_init_guess'
opt.lr_optim = 1e-5
opt.need_single_image_normalization = True
opt.need_spectral_norm_error_model = True
opt.need_spectral_norm_upstream = False
opt.optim_criterion = 'mse'
opt.optim_iters = 20
opt.optim_method = 'stn'
opt.optim_type = 'adam'
opt.out_dir = './out'
opt.prevent_neg = 'sigmoid'
opt.template_path = './data/world_cup_template.png'
opt.warp_dim = 8
opt.warp_type = 'homography'

# read original image
goal_image = imageio.imread(opt.goal_image_path, pilmode='RGB')
# resize image to square shape, 256 * 256, and squash to [0, 1]
pil_image = Image.fromarray(np.uint8(goal_image))
pil_image = pil_image.resize([256, 256], resample=Image.NEAREST)
goal_image = np.array(pil_image)

# covert np image to torch image, and do normalization
goal_image = utils.np_img_to_torch_img(goal_image)
if opt.need_single_image_normalization:
    goal_image = image_utils.normalize_single_image(goal_image)
print('mean of goal image: {0}'.format(goal_image.mean()))
print('std of goal image: {0}'.format(goal_image.std()))

# read template image
template_image = imageio.imread(opt.template_path, pilmode='RGB')
template_image = template_image / 255.0
if opt.coord_conv_template:
    template_image = image_utils.rgb_template_to_coord_conv_template(template_image)

# covert np image to torch image, and do normalization
template_image = utils.np_img_to_torch_img(template_image)
if opt.need_single_image_normalization:
    template_image = image_utils.normalize_single_image(template_image)
print('mean of template: {0}'.format(template_image.mean()))
print('std of template: {0}'.format(template_image.std()))

e2e = end_2_end_optimization.End2EndOptimFactory.get_end_2_end_optimization_model(opt)

orig_homography, optim_homography = e2e.optim(goal_image[None], template_image)

# reload image and template for visualization
# overload goal image
goal_image_draw = imageio.imread(opt.goal_image_path, pilmode='RGB')
goal_image_draw = goal_image_draw / 255.0
outshape = goal_image_draw.shape[0:2]

# overload template image
template_image_draw = imageio.imread(opt.template_path, pilmode='RGB')
template_image_draw = template_image_draw / 255.0
template_image_draw = image_utils.rgb_template_to_coord_conv_template(template_image_draw)
template_image_draw = utils.np_img_to_torch_img(template_image_draw)


# warp template image with optimized guess
warped_tmp_optim = warp.warp_image(template_image_draw, optim_homography, out_shape=outshape)[0]
warped_tmp_optim = utils.torch_img_to_np_img(warped_tmp_optim)

# show optimized guess overlay
show_image = np.copy(goal_image_draw)
valid_index = warped_tmp_optim[:, :, 0] > 0.0
overlay = (goal_image_draw[valid_index].astype('float32') + warped_tmp_optim[valid_index].astype('float32'))/2
show_image[valid_index] = overlay

# show optimized top-down view
H_inv = torch.inverse(optim_homography)
outshape = template_image_draw.shape[1:3]
warped_frm = warp.warp_image(utils.np_img_to_torch_img(goal_image_draw)[None], H_inv, out_shape=outshape)[0]
# plt.imshow(utils.torch_img_to_np_img(warped_frm)*0.5+utils.torch_img_to_np_img(template_image_draw)*0.5)
# plt.savefig("./after_trans.jpg")
# plt.close('all')

# this is needed to add into the new demo

# warp a point from frame to template
frame_point = np.array([568, 488])
# plt.imshow(imageio.imread(opt.goal_image_path, pilmode='RGB'))
# plt.scatter(frame_point[0], frame_point[1])
# plt.savefig("./out_pre.jpg")
# plt.close('all')

x = torch.tensor(frame_point[0] / 1280 - 0.5).float()
y = torch.tensor(frame_point[1] / 720 - 0.5).float()
xy = torch.stack([x, y, torch.ones_like(x)])
xy_warped = torch.matmul(optim_homography.cpu(), xy)  # H.bmm(xy)
xy_warped, z_warped = xy_warped.split(2, dim=1)

# we multiply by 2, since our homographies map to
# coordinates in the range [-0.5, 0.5] (the ones in our GT datasets)
xy_warped = 2.0 * xy_warped / (z_warped + 1e-8)
x_warped, y_warped = torch.unbind(xy_warped, dim=1)
# [-1, 1] -> [0, 1]
x_warped = (x_warped.item() * 0.5 + 0.5) * 1050
y_warped = (y_warped.item() * 0.5 + 0.5) * 680

plt.imshow(utils.torch_img_to_np_img(warped_frm)*0.5+utils.torch_img_to_np_img(template_image_draw)*0.5)
plt.scatter(x_warped, y_warped)
plt.savefig("./out.jpg")
plt.close('all')
