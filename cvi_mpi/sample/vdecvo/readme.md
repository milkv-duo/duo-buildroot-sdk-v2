## sample_vdecvo instructions



| sample | usage | purpose | res |
| ------ | ----- | ------- | --- |
| 0 VDEC channel - BIND - VPSS - BIND - VO | ./sample_vdecvo 0 res/enc-1080p.264 | Test bind mode | NULL |
| 1 VDEC channel - SEND - VPSS - SEND - VO | ./sample_vdecvo 1 res/enc-1080p.264 | Test send frame mode | NULL |
| 1 VDEC channel - SEND - VPSS - SEND - VO | ./sample_vdecvo 1 res/enc-1080p.264 res/enc-1080p.264 | Test multi VDEC channel send frame mode | NULL |
| 2 VDEC channel - BIND - VENC | ./sample_vdecvo 2 res/enc-1080p.264 | Test bind mode | NULL |
| 3 VDEC channel - BIND - VO | ./sample_vdecvo 3 res/enc-720p.264 | Test bind mode | NULL |
