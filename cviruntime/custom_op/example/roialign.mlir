

module {
  func @tpu_func(%arg0 : tensor<1x512x38x50xf32>, %arg1 : tensor<1x1x300x5xf32>) -> tensor<300x512x7x7xf32> {
    %0 = "tpu.weight_file"() {filename = "roialign_1_06eeeb7e.npz"} : () -> memref<10xf32>
    %1 = "tpu.input"(%arg0) {name = "data0", quant = {is_asymmetric = false, is_perchannel = false, mode = "NONE", param_type = "NONE", threshold_max = 0.000000e+00 : f32, threshold_min = 0.000000e+00 : f32, zero_point = 0 : i32}} : (tensor<1x512x38x50xf32>) -> tensor<1x512x38x50xf32>
    %2 = "tpu.input"(%arg1) {name = "data1", quant = {is_asymmetric = false, is_perchannel = false, mode = "NONE", param_type = "NONE", threshold_max = 0.000000e+00 : f32, threshold_min = 0.000000e+00 : f32, zero_point = 0 : i32}} : (tensor<1x1x300x5xf32>) -> tensor<1x1x300x5xf32>
    %3 = "tpu.custom_op"(%1, %2) {name = "roi_align", operation_name = "roialign", param = {pooled_h = 7 : i32, pooled_w = 7 : i32, spatial_scale = 6.250000e-02 : f32}, quant = {is_asymmetric = false, is_perchannel = false, mode = "NONE", param_type = "NONE", threshold_max = 0.000000e+00 : f32, threshold_min = 0.000000e+00 : f32, zero_point = 0 : i32}} : (tensor<1x512x38x50xf32>, tensor<1x1x300x5xf32>) -> tensor<300x512x7x7xf32>
    return %3 : tensor<300x512x7x7xf32>
  }
}
