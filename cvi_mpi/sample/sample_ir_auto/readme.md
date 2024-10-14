## sample_ir_auto 使用说明:

sample_ir_auto 为软光敏功能的标定和功能函数CVI_ISP_IrAutoRunOnce()的使用示例程序
使用步骤：
1、标定：获取开关ir 灯时对应的ISO、RGain、BGain;
2、功能验证：使用标定获取的参数， 验证ir auto 功能是否正常。

使用示例：
    1、标定 （获取当前 ISO、RGain、BGain）
    ./sample_ir_auto 0
    2、设置ir auto参数验证功能是否正常
    ./sample_ir_auto 1 16000 400 280 190 280 190 0 (<mode> <u32Normal2IrIsoThr> <u32Ir2NormalIsoThr> <u32RGMax> <u32RGMin> <u32BGMax> <u32BGMin> <enIrStatus>)