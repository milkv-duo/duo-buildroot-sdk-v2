# Gaussian Blur

In this example, we'll use ``uint8`` to blur and image.

## Formula breakdown

The formula of Gaussian kernel,

$$G(x) = \frac{1}{16}
         \begin{bmatrix}
         1 & 2 & 1 \\
         2 & 4 & 2 \\
         1 & 2 & 1
         \end{bmatrix} * I$$

We will use ``tiu_depthwise_convolution`` to get the result.

### ``chl_quan_param``

``chl_quan_param`` is a quantized multiplier parameter used in ``tiu_depthwise_convolution``. The parameter accepts any quantized floating point number between (0,1].

## Used OP

1. Depthwise convolution

## Parameter setup

```c++
  // cvk_tiu_depthwise_convolution_param_t
  cvk_tl_shape_t packed_s = {1, tl_shape.c, 1, MULTIPLIER_ONLY_PACKED_DATA_SIZE};
  // tl_multiplier and bmmem must be passed to the end and freed with  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_multiplier); and bmmem_device_free(*ctx, bmmem);
  cvk_tl_t *tl_multiplier = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, packed_s, CVK_FMT_U8, 1);
  bmmem_device_t bmmem;
  // tg must be passed and freed after tdma_g2l_bf16_tensor_copy is done.
  cvk_tg_t *tg = malloc(sizeof(cvk_tg_t));
  // Copy multiplier_arr into tl_multiplier.
  {
    // Create a tg memory.
    CVI_U32 size = tl_shape.c * MULTIPLIER_ONLY_PACKED_DATA_SIZE;
    bmmem = bmmem_device_alloc_raw(*ctx, size);
    tg->start_address = bmmem_device_addr(bmmem);
    tg->base_reg_index = 0;
    tg->fmt = tl_multiplier->fmt;
    tg->shape = {tl_multiplier->shape.n, tl_multiplier->shape.c, tl_multiplier->shape.h, tl_multiplier->shape.w};
    tg->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, tg->shape, tg->fmt);

    // Get chl_quan_parameters.
    float multiplier = 0.8f;
    CVI_U32 quantized_multiplier;
    int right_shift;
    cvm_get_chl_quan(multiplier, &quantized_multiplier, &right_shift);
    vaddr = (uint8_t *)bmmem_device_v_addr(bmmem);
    // Fill parameters into tg memory.
    void cvm_fill_chl_quan_data(tg->shape.c, quantized_multiplier, right_shift,
                                vaddr, NULL, false);
    bmmem_device_invld(*ctx, bmmem);
    // tg 2 tl operation.
    cvk_tdma_g2l_tensor_copy_param_t p;
    p.src = tg;
    p.dst = tl_multiplier;
    cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &p);
  }
  // Reshape chl_quan to specific shape.
  tl_multiplier->shape = {1, tl_shape.c, 1, 1};  // Magic number.
  tl_multiplier->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, tl_multiplier->shape, tl_multiplier->fmt, 0);

  // Pad 1 in four directions for a 3x3 kernel.
  m_p_conv.pad_top = 1;
  m_p_conv.pad_bottom = 1;
  m_p_conv.pad_left = 1;
  m_p_conv.pad_right = 1;
  m_p_conv.stride_w = 1;
  m_p_conv.stride_h = 1;
  m_p_conv.relu_enable = 1;
  m_p_conv.ins_h = 0;
  m_p_conv.ins_w = 0;
  m_p_conv.ins_last_h = 0;
  m_p_conv.ins_last_w = 0;
  m_p_conv.dilation_h = 1;
  m_p_conv.dilation_w = 1;
  m_p_conv.has_bias = 0;

  m_p_conv.ifmap = tl_input;
  m_p_conv.ofmap = tl_output;
  m_p_conv.weight = tl_kernel;
  m_p_conv.chl_quan_param = tl_multiplier;
```

## TPU operations

```c++
  cvk_ctx->ops->tiu_depthwise_convolution(cvk_ctx, &m_p_conv);
```