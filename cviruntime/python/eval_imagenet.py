#!/usr/bin/env python3
# use pytorch for dataloader
# https://github.com/pytorch/examples/blob/master/imagenet/main.py

import argparse
import os
import random
import shutil
import time
import warnings
import numpy as np
import pyruntime
from PIL import Image
from PIL import ImageFile
ImageFile.LOAD_TRUNCATED_IMAGES = True

import torch
import torch.nn as nn
import torch.utils.data
import torchvision.transforms as transforms
import torchvision.datasets as datasets

class AverageMeter(object):
    """Computes and stores the average and current value"""
    def __init__(self, name, fmt=':f'):
        self.name = name
        self.fmt = fmt
        self.reset()

    def reset(self):
        self.val = 0
        self.avg = 0
        self.sum = 0
        self.count = 0

    def update(self, val, n=1):
        self.val = val
        self.sum += val * n
        self.count += n
        self.avg = self.sum / self.count

    def __str__(self):
        fmtstr = '{name} {val' + self.fmt + '} ({avg' + self.fmt + '})'
        return fmtstr.format(**self.__dict__)

class ProgressMeter(object):
    def __init__(self, num_batches, meters, prefix=""):
        self.batch_fmtstr = self._get_batch_fmtstr(num_batches)
        self.meters = meters
        self.prefix = prefix

    def display(self, batch):
        entries = [self.prefix + self.batch_fmtstr.format(batch)]
        entries += [str(meter) for meter in self.meters]
        print('\t'.join(entries))

    def _get_batch_fmtstr(self, num_batches):
        num_digits = len(str(num_batches // 1))
        fmt = '{:' + str(num_digits) + 'd}'
        return '[' + fmt + '/' + fmt.format(num_batches) + ']'

def accuracy(output, target, topk=(1,)):
    """Computes the accuracy over the k top predictions for the specified values of k"""
    maxk = max(topk)
    batch_size = target.size(0)

    _, pred = output.topk(maxk, 1, True, True)
    pred = pred.t()
    correct = pred.eq(target.view(1, -1).expand_as(pred))

    res = []
    for k in topk:
        correct_k = correct[:k].view(-1).float().sum(0, keepdim=True)
        res.append(correct_k.mul_(100.0 / batch_size))
    return res

def datasetLoader(args):
  image_resize_dims = [int(s) for s in args.image_resize_dims.split(',')]
  net_input_dims = [int(s) for s in args.net_input_dims.split(',')]
  image_resize_dims = [ max(x,y) for (x,y) in zip(image_resize_dims, net_input_dims)]

  valdir = os.path.join(args.dataset, 'val')
  if (args.loader_transforms):
    normalize = transforms.Normalize(mean=[0.485, 0.456, 0.406],
                                     std=[0.229, 0.224, 0.225])
    val_loader = torch.utils.data.DataLoader(
        datasets.ImageFolder(valdir, transforms.Compose([
            transforms.Resize(image_resize_dims),
            transforms.CenterCrop(net_input_dims),
            transforms.ToTensor(),
            normalize,
        ])),
        batch_size=args.batch_size, shuffle=True)
  else:
    val_loader = torch.utils.data.DataLoader(
        datasets.ImageFolder(valdir, transforms.Compose([
            transforms.Resize(image_resize_dims),
            transforms.CenterCrop(net_input_dims),
            transforms.ToTensor()
        ])),
        batch_size=args.batch_size, shuffle=True)
  return val_loader

def imagePreprocssing(args, images, mean, qscale):
  inputs = np.array([])
  for image in images:
    if args.loader_transforms:
      # loader do normalize already
      x = image.numpy()
    else:
      # pytorch ToTensor() will do HWC to CHW, and change range to [0.0, 1.0]
      # for pytorch, seeing errors if not include ToTensor in transforms
      # change to range [0, 255]
      x = image.numpy() * 255
      x = x.astype('uint8')
      # transposed already in ToTensor()
      # x = np.transpose(x, (2, 0, 1))
      # still need the swap for caffe models
      x = x[[2,1,0], :, :]
      x = x.astype(np.float32)
      if args.raw_scale != 255.0:
        x = x * args.raw_scale / 255.0
      # apply mean
      if mean.size != 0:
        x -= mean
      if qscale != 0:
        x = x * qscale
    # expand to 4-D again
    x = np.expand_dims(x, axis=0)
    if inputs.size:
      inputs = np.append(inputs, x, axis=0)
    else:
      inputs = x

  if args.input_scale != 1.0:
    inputs *= args.input_scale
  return inputs

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description="Classification Evaluation on ImageNet Dataset.")
  parser.add_argument("--cvimodel", type=str)
  parser.add_argument("--batch_size", type=int, default=1)
  parser.add_argument("--dataset", type=str, help="The root directory of the ImageNet dataset.")
  parser.add_argument("--image_resize_dims", type=str, default='256,256')
  parser.add_argument("--net_input_dims", type=str, default='224,224')
  parser.add_argument("--raw_scale", type=float, help="Multiply raw input image data by this scale.", default=255.0)
  parser.add_argument("--mean", help="Per Channel image mean values")
  parser.add_argument("--input_scale", type=float, help="Multiply input features by this scale.", default=1.0)
  parser.add_argument("--count", type=int, default=50000)
  parser.add_argument("--loader_transforms", type=int, help="image transform by torch loader", default=0)
  args = parser.parse_args()

  if args.mean:
    mean = np.array([float(s) for s in args.mean.split(',')], dtype=np.float32)
    mean = mean[:, np.newaxis, np.newaxis]
  else:
    mean = np.array([])

  # load model
  model = pyruntime.Model(args.cvimodel, args.batch_size)
  print('load model {}'.format(args.cvimodel))

  val_loader = datasetLoader(args)

  batch_time = AverageMeter('Time', ':6.3f')
  losses = AverageMeter('Loss', ':.4e')
  top1 = AverageMeter('Acc@1', ':6.2f')
  top5 = AverageMeter('Acc@5', ':6.2f')
  progress = ProgressMeter(len(val_loader) * args.batch_size,
                           [batch_time, losses, top1, top5],
                           prefix='Test: ')

  # define loss function (criterion) and optimizer
  criterion = nn.CrossEntropyLoss()
  threshold = ((50 + args.batch_size - 1) // args.batch_size) * args.batch_size
  total = len(val_loader) * args.batch_size
  count = 0
  end = time.time()
  for i, (images, target) in enumerate(val_loader):
    # preprocessing
    x = imagePreprocssing(args, images, mean, model.inputs[0].qscale)
    # inference
    model.inputs[0].data[:] = x
    model.forward()

    # validate output prob
    assert(len(model.outputs) == 1)
    res = model.outputs[0].data
    prob = np.reshape(res, (res.shape[0], res.shape[1]))
    output = torch.from_numpy(prob)

    # loss
    loss = criterion(output, target)

    # measure accuracy and record loss
    acc1, acc5 = accuracy(output, target, topk=(1, 5))
    losses.update(loss.item(), images.size(0))
    top1.update(acc1[0], images.size(0))
    top5.update(acc5[0], images.size(0))

    # measure elapsed time
    batch_time.update(time.time() - end)
    end = time.time()

    count += args.batch_size
    if count % threshold == 0:
      progress.display(count)
    if count >= args.count:
      progress.display(count)
      break
    if count + args.batch_size > total:
      progress.display(count)
      break
  print(' * Acc@1 {top1.avg:.3f} Acc@5 {top5.avg:.3f}'
        .format(top1=top1, top5=top5))
