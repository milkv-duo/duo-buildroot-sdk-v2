import torch
import torch.nn as nn
import numpy as np

class Light(nn.Module):
    def __init__(self, kernel_size=15):
        super().__init__()
        self.pool = nn.MaxPool2d(kernel_size=kernel_size, stride=1, padding=(kernel_size - 1) // 2)

    def forward(self, x):
        bkg = self.pool(x)
        x = torch.where(bkg > x, 255 - (bkg - x), torch.tensor(255.))
        return x


if __name__ == '__main__':
    net = Light()
    shape = (1, 1, 40, 70)
    x = torch.randint(255, shape)
    x = x.to(torch.float32)
    np.savez("light_x.npz", x=x.numpy().astype(np.uint8))
    print("x", x.shape, x)
    out = net(x)
    np.savez("light_out.npz", out=out.numpy().astype(np.uint8))
    print("out", out.shape, out)