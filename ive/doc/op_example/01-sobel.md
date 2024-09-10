# Sobel

The dynamic range of Sobel is larger than char, so this example is done using BF16 operations.

## Formula breakdown

The formula of Sobel,

$$G(x) = \begin{bmatrix}
         +1 & 0 & -1 \\
         +2 & 0 & -2 \\
         +1 & 0 & -1
         \end{bmatrix} * I$$
$$G(y) = \begin{bmatrix}
         +1 & +2 & +1 \\
         0 & 0 & 0 \\
         -1 & -2 & -1
         \end{bmatrix} * I$$
$$G = |G(x)| + |G(y)|$$

To calculate approximations of the derivatives, we use ``tiu_bf16_depthwise_convolution``. We let

$$tl\_kernel\_gx = \begin{bmatrix}
                   +1 & 0 & -1 \\
                   +2 & 0 & -2 \\
                   +1 & 0 & -1
                   \end{bmatrix}$$

$$tl\_kernel\_gy = \begin{bmatrix}
                   +1 & +2 & +1 \\
                   0 & 0 & 0 \\
                   -1 & -2 & -1
                   \end{bmatrix}$$

For the resulting gradient approximations, we use ``tiu_bf16_mul`` and ``tiu_bf16_max`` to get the absolute value , and ``tiu_bf16_add`` to get the final result.

$$G(x)' = G(x) * (-1)$$

$$|G(x)| = max(G(x), G(x)')$$

$$G(y)' = G(y) * (-1)$$

$$|G(y)| = max(G(y), G(y)')$$

$$G = |G(x)| + |G(y)|$$

## Used OP

1. Depthwise convolution
2. Mul
3. Max
4. Add

## Parameter setup

```c++
  // depthwise convolution parameter
  // cvk_tiu_depthwise_pt_convolution_param_t
  // Pad 1 in four directions for a 3x3 kernel.
  conv_x.pad_top = 1;
  conv_x.pad_bottom = 1;
  conv_x.pad_left = 1;
  conv_x.pad_right = 1;

  conv_x.stride_w = 1;
  conv_x.stride_h = 1;
  conv_x.relu_enable = 0;
  conv_x.ins_h = 0;
  conv_x.ins_w = 0;
  conv_x.ins_last_h = 0;
  conv_x.ins_last_w = 0;
  conv_x.dilation_h = 1;
  conv_x.dilation_w = 1;
  // No bias required.
  conv_x.bias = NULL;
  conv_x.rshift_bits = 0;

  // tl_gx = depthwise_conv(tl_input, tl_kernel_gx)
  conv_x.ifmap = tl_input;
  conv_x.ofmap = tl_gx;
  conv_x.weight = tl_kernel_gx;

  // tl_gy = depthwise_conv(tl_input, tl_kernel_gy)
  conv_y = m_p_conv_x;
  conv_y.ofmap = tl_gy;
  conv_y.weight = tl_kernel_gy;

  // G(x)' = G(x) * (-1);
  mul_a.rshift_bits = 0;
  mul_a.relu_enable = 0;
  mul_a.res_high = NULL;
  mul_a.res_low = tl_gy;
  mul_a.a = tl_gx;
  mul_a.b_is_const = 1;
  mul_a.b_const.val = convert_fp32_bf16(-1.f);

  // abs(G(x)) = max(G(x), G(x)');
  max_a.b_is_const = 0;
  max_a.a = tl_gx;
  max_a.b = tl_gy;
  max_a.max = tl_gx;

  // G(y)' = G(y) * (-1);
  mul_b.rshift_bits = 0;
  mul_b.relu_enable = 0;
  mul_b.res_high = NULL;
  mul_b.res_low = tl_buf;
  mul_b.a = tl_gy;
  mul_b.b_is_const = 1;
  mul_b.b_const.val = convert_fp32_bf16(-1.f);

  // abs(G(y)) = max(G(y), G(y)');
  max_b.b_is_const = 0;
  max_b.a = tl_gy;
  max_b.b = tl_buf;
  max_b.max = tl_gy;

  // G = G(x) + G(y)
  add.relu_enable = 0;
  add.b_is_const = 0;
  add.rshift_bits = 0;
  add.a_low = tl_gx;
  add.a_high = NULL;
  add.b_low = tl_gy;
  add.b_high = NULL;
  add.res_low = tl_gx;
  add.res_high = NULL;
```

## TPU operations

```c++
  // tl_input -> tl_gx
  cvk_ctx->ops->tiu_pt_depthwise_convolution(cvk_ctx, &m_p_conv_x);
  cvk_ctx->ops->tiu_mul(cvk_ctx, &m_p_mul_a);
  cvk_ctx->ops->tiu_max(cvk_ctx, &m_p_max_a);
  // tl_input -> tl_gy
  cvk_ctx->ops->tiu_pt_depthwise_convolution(cvk_ctx, &m_p_conv_y);
  cvk_ctx->ops->tiu_mul(cvk_ctx, &m_p_mul_b);
  cvk_ctx->ops->tiu_max(cvk_ctx, &m_p_max_b);

  // tl_gx + tl_gy
  cvk_ctx->ops->tiu_add(cvk_ctx, &m_p_add);
```