# --------------------------------------------------------
# Pytorch Faster R-CNN and FPN
# Licensed under The MIT License [see LICENSE for details]
# Written by Zheqi He and Xinlei Chen, Yixiao Ge
# --------------------------------------------------------
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from .network import Network
from .config import cfg

# import utils.timer

import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.autograd import Variable
import math
import torch.utils.model_zoo as model_zoo


__all__ = ['ResNet', 'resnet18', 'resnet34', 'resnet50', 'resnet101',
       'resnet152']


model_urls = {
  'resnet18': 'https://s3.amazonaws.com/pytorch/models/resnet18-5c106cde.pth',
  'resnet34': 'https://s3.amazonaws.com/pytorch/models/resnet34-333f7ec4.pth',
  'resnet50': 'https://s3.amazonaws.com/pytorch/models/resnet50-19c8e357.pth',
  'resnet101': 'https://s3.amazonaws.com/pytorch/models/resnet101-5d3b4d8f.pth',
  'resnet152': 'https://s3.amazonaws.com/pytorch/models/resnet152-b121ed2d.pth',
}


def conv3x3(in_planes, out_planes, stride=1):
  "3x3 convolution with padding"
  return nn.Conv2d(in_planes, out_planes, kernel_size=3, stride=stride,
           padding=1, bias=False)


class BasicBlock(nn.Module):
  expansion = 1

  def __init__(self, inplanes, planes, stride=1, downsample=None):
    super(BasicBlock, self).__init__()
    self.conv1 = conv3x3(inplanes, planes, stride)
    self.bn1 = nn.BatchNorm2d(planes)
    self.relu = nn.ReLU(inplace=True)
    self.conv2 = conv3x3(planes, planes)
    self.bn2 = nn.BatchNorm2d(planes)
    self.downsample = downsample
    self.stride = stride

  def forward(self, x):
    residual = x

    out = self.conv1(x)
    out = self.bn1(out)
    out = self.relu(out)

    out = self.conv2(out)
    out = self.bn2(out)

    if self.downsample is not None:
      residual = self.downsample(x)

    out += residual
    out = self.relu(out)

    return out


class Bottleneck(nn.Module):
  expansion = 4

  def __init__(self, inplanes, planes, stride=1, downsample=None):
    super(Bottleneck, self).__init__()
    self.conv1 = nn.Conv2d(inplanes, planes, kernel_size=1, stride=stride, bias=False) # change
    self.bn1 = nn.BatchNorm2d(planes)
    self.conv2 = nn.Conv2d(planes, planes, kernel_size=3, stride=1, # change
                 padding=1, bias=False)
    self.bn2 = nn.BatchNorm2d(planes)
    self.conv3 = nn.Conv2d(planes, planes * 4, kernel_size=1, bias=False)
    self.bn3 = nn.BatchNorm2d(planes * 4)
    self.relu = nn.ReLU(inplace=True)
    self.downsample = downsample
    self.stride = stride

  def forward(self, x):
    residual = x

    out = self.conv1(x)
    out = self.bn1(out)
    out = self.relu(out)

    out = self.conv2(out)
    out = self.bn2(out)
    out = self.relu(out)

    out = self.conv3(out)
    out = self.bn3(out)

    if self.downsample is not None:
      residual = self.downsample(x)

    out += residual
    out = self.relu(out)

    return out

class BuildBlock(nn.Module):
  def __init__(self, planes=256):
    super(BuildBlock, self).__init__()
    # Top-down layers, use nn.ConvTranspose2d to replace nn.Conv2d+F.upsample?
    self.toplayer1 = nn.Conv2d(2048, planes, kernel_size=1, stride=1, padding=0)  # Reduce channels
    self.toplayer2 = nn.Conv2d( 256, planes, kernel_size=3, stride=1, padding=1)
    self.toplayer3 = nn.Conv2d( 256, planes, kernel_size=3, stride=1, padding=1)
    self.toplayer4 = nn.Conv2d( 256, planes, kernel_size=3, stride=1, padding=1)

    # Lateral layers
    self.latlayer1 = nn.Conv2d(1024, planes, kernel_size=1, stride=1, padding=0)
    self.latlayer2 = nn.Conv2d( 512, planes, kernel_size=1, stride=1, padding=0)
    self.latlayer3 = nn.Conv2d( 256, planes, kernel_size=1, stride=1, padding=0)

    self.subsample = nn.AvgPool2d(2, stride=2)

  def _upsample_add(self, x, y):
    _,_,H,W = y.size()
    return F.upsample(x, size=(H,W), mode='bilinear') + y

  def forward(self, c2, c3, c4, c5):
    # Top-down
    p5 = self.toplayer1(c5)
    p6 = self.subsample(p5)
    p4 = self._upsample_add(p5, self.latlayer1(c4))
    p4 = self.toplayer2(p4)
    p3 = self._upsample_add(p4, self.latlayer2(c3))
    p3 = self.toplayer3(p3)
    p2 = self._upsample_add(p3, self.latlayer3(c2))
    p2 = self.toplayer4(p2)

    return p2, p3, p4, p5, p6

class HiddenBlock(nn.Module):
  def __init__(self, channels, planes):
    super(HiddenBlock, self).__init__()
    self.fc1 = nn.Linear(channels * 7 * 7,planes)
    self.fc2 = nn.Linear(planes,planes)

  def forward(self, x):
    x = self.fc1(x)
    x = F.relu(x)
    x = self.fc2(x)
    x = F.relu(x)
    return x

class ResNet(nn.Module):
  def __init__(self, block, layers, num_classes=1000):
    self.inplanes = 64
    super(ResNet, self).__init__()
    self.conv1 = nn.Conv2d(3, 64, kernel_size=7, stride=2, padding=3,
                 bias=False)
    self.bn1 = nn.BatchNorm2d(64)
    self.relu = nn.ReLU(inplace=True)
    # maxpool different from pytorch-resnet, to match tf-faster-rcnn
    self.maxpool = nn.MaxPool2d(kernel_size=3, stride=2, padding=1)
    self.layer1 = self._make_layer(block, 64, layers[0])
    self.layer2 = self._make_layer(block, 128, layers[1], stride=2)
    self.layer3 = self._make_layer(block, 256, layers[2], stride=2)
    # use stride 1 for the last conv4 layer (same as tf-faster-rcnn)
    if cfg.FPN:
      self.layer4 = self._make_layer(block, 512, layers[3], stride=2)
    else:
      self.layer4 = self._make_layer(block, 512, layers[3], stride=1)

    for m in self.modules():
      if isinstance(m, nn.Conv2d):
        n = m.kernel_size[0] * m.kernel_size[1] * m.out_channels
        m.weight.data.normal_(0, math.sqrt(2. / n))
      elif isinstance(m, nn.BatchNorm2d):
        m.weight.data.fill_(1)
        m.bias.data.zero_()

  def _make_layer(self, block, planes, blocks, stride=1):
    downsample = None
    if stride != 1 or self.inplanes != planes * block.expansion:
      downsample = nn.Sequential(
        nn.Conv2d(self.inplanes, planes * block.expansion,
              kernel_size=1, stride=stride, bias=False),
        nn.BatchNorm2d(planes * block.expansion),
      )

    layers = []
    layers.append(block(self.inplanes, planes, stride, downsample))
    self.inplanes = planes * block.expansion
    for i in range(1, blocks):
      layers.append(block(self.inplanes, planes))

    return nn.Sequential(*layers)

def resnet18(pretrained=False):
  """Constructs a ResNet-18 model.
  Args:
    pretrained (bool): If True, returns a model pre-trained on ImageNet
  """
  model = ResNet(BasicBlock, [2, 2, 2, 2])
  if pretrained:
    model.load_state_dict(model_zoo.load_url(model_urls['resnet18']))
  return model


def resnet34(pretrained=False):
  """Constructs a ResNet-34 model.
  Args:
    pretrained (bool): If True, returns a model pre-trained on ImageNet
  """
  model = ResNet(BasicBlock, [3, 4, 6, 3])
  if pretrained:
    model.load_state_dict(model_zoo.load_url(model_urls['resnet34']))
  return model


def resnet50(pretrained=False):
  """Constructs a ResNet-50 model.
  Args:
    pretrained (bool): If True, returns a model pre-trained on ImageNet
  """
  model = ResNet(Bottleneck, [3, 4, 6, 3])
  if pretrained:
    model.load_state_dict(model_zoo.load_url(model_urls['resnet50']))
  return model


def resnet101(pretrained=False):
  """Constructs a ResNet-101 model.
  Args:
    pretrained (bool): If True, returns a model pre-trained on ImageNet
  """
  model = ResNet(Bottleneck, [3, 4, 23, 3])
  if pretrained:
    model.load_state_dict(model_zoo.load_url(model_urls['resnet101']))
  return model


def resnet152(pretrained=False):
  """Constructs a ResNet-152 model.
  Args:
    pretrained (bool): If True, returns a model pre-trained on ImageNet
  """
  model = ResNet(Bottleneck, [3, 8, 36, 3])
  if pretrained:
    model.load_state_dict(model_zoo.load_url(model_urls['resnet152']))
  return model

class resnetv1(Network):
  def __init__(self, num_layers=50):
    Network.__init__(self)
    if cfg.FPN:
      self._feat_stride = [4, 8, 16, 32, 64]
      self._net_conv_channels = 256
      self._fc7_channels = 1024
    else:
      self._feat_stride = [16, ]
      self._net_conv_channels = 1024
      self._fc7_channels = 2048
    self._feat_compress = [1. / float(self._feat_stride[0]), ]
    self._num_layers = num_layers

  def _crop_pool_layer(self, bottom, rois):
    if cfg.FPN:
      return Network._crop_pool_layer_fpn(self, bottom, rois, cfg.RESNET.MAX_POOL)
    else:
      return Network._crop_pool_layer(self, bottom, rois, cfg.RESNET.MAX_POOL)

  def _image_to_head(self):
    if cfg.FPN:
      c2 = self._layers['head'][0](self._image)
      c3 = self._layers['head'][1](c2)
      c4 = self._layers['head'][2](c3)
      c5 = self._layers['head'][3](c4)
      p2, p3, p4, p5, p6 = self._layers['fpn'](c2, c3, c4, c5)
      net_conv = [p2, p3, p4, p5, p6]
    else:
      net_conv = self._layers['head']

    self._act_summaries['conv'] = net_conv
    return net_conv

  def _head_to_tail(self, pool5):
    if cfg.FPN:
      fc7 = self.head(pool5)
    else:
      fc7 = self.resnet.layer4(pool5).mean(3).mean(2) # average pooling after layer4
    return fc7

  def _init_head_tail(self, resnet):
    self.resnet = resnet
    '''
    # choose different blocks for different number of layers
    if self._num_layers == 50:
      self.resnet = resnet50()

    elif self._num_layers == 101:
      self.resnet = resnet101()

    elif self._num_layers == 152:
      self.resnet = resnet152()

    else:
      # other numbers are not supported
      raise NotImplementedError
    '''

    # Fix blocks 
    for p in self.resnet.bn1.parameters(): p.requires_grad=False
    for p in self.resnet.conv1.parameters(): p.requires_grad=False
    assert (0 <= cfg.RESNET.FIXED_BLOCKS < 4)
    if cfg.RESNET.FIXED_BLOCKS >= 3:
      for p in self.resnet.layer3.parameters(): p.requires_grad=False
    if cfg.RESNET.FIXED_BLOCKS >= 2:
      for p in self.resnet.layer2.parameters(): p.requires_grad=False
    if cfg.RESNET.FIXED_BLOCKS >= 1:
      for p in self.resnet.layer1.parameters(): p.requires_grad=False

    def set_bn_fix(m):
      classname = m.__class__.__name__
      if classname.find('BatchNorm') != -1:
        for p in m.parameters(): p.requires_grad=False

    self.resnet.apply(set_bn_fix)

    # Build resnet.
    if cfg.FPN:
    # Build Building Block for FPN
      self.fpn_block = BuildBlock()
      self._layers['fpn'] = self.fpn_block
      self._layers['head'] = []
      self._layers['head'].append(nn.Sequential(self.resnet.conv1, self.resnet.bn1,self.resnet.relu, 
        self.resnet.maxpool,self.resnet.layer1))
      self._layers['head'].append(nn.Sequential(self.resnet.layer2))
      self._layers['head'].append(nn.Sequential(self.resnet.layer3))
      self._layers['head'].append(nn.Sequential(self.resnet.layer4))
      self.head = HiddenBlock(self._net_conv_channels,self._fc7_channels)
    else:
      self._layers['head'] = nn.Sequential(self.resnet.conv1, self.resnet.bn1,self.resnet.relu, 
        self.resnet.maxpool,self.resnet.layer1,self.resnet.layer2,self.resnet.layer3)

  def train(self, mode=True):
    # Override train so that the training mode is set as we want
    nn.Module.train(self, mode)
    if mode:
      # Set fixed blocks to be in eval mode (not really doing anything)
      self.resnet.eval()
      if cfg.RESNET.FIXED_BLOCKS <= 3:
        self.resnet.layer4.train()
      if cfg.RESNET.FIXED_BLOCKS <= 2:
        self.resnet.layer3.train()
      if cfg.RESNET.FIXED_BLOCKS <= 1:
        self.resnet.layer2.train()
      if cfg.RESNET.FIXED_BLOCKS == 0:
        self.resnet.layer1.train()

      # Set batchnorm always in eval mode during training
      def set_bn_eval(m):
        classname = m.__class__.__name__
        if classname.find('BatchNorm') != -1:
          m.eval()

      self.resnet.apply(set_bn_eval)

  def load_pretrained_cnn(self, state_dict):
    self.resnet.load_state_dict({k: state_dict[k] for k in list(self.resnet.state_dict())})

if __name__ == "__main__":

    net = resnet50()
    for k, v in net.state_dict().items():
        print(k, v.shape)