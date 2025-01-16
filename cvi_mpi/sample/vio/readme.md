## vio sample instructions



| sample                                                   | usage                                                                                              | purpose                                                                                   | res                |
| -------------------------------------------------------- | -------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------- | ------------------ |
| VI(Offline)-VPSS(Offline)-VO(Rotation)                   | ./sample_vio 0   sensor and panel required  need to place the sensor_cfg.ini in the path /mnt/data | Implement pathways VI(Offline)-VPSS(Offline)-VO and rotate in vo                          | NULL               |
| VI(Offline)-VPSS(Offline,Keep Aspect Ratio)-VO           | ./sample_vio 1   sensor and panel required  need to place the sensor_cfg.ini in the path /mnt/data | Implement pathways VI(Offline)-VPSS(Offline)-VO and vpss modify aspect ratio              | NULL               |
| VI(Offline, Rotation)-VPSS(Offline,Keep Aspect Ratio)-VO | ./sample_vio 2   sensor and panel required  need to place the sensor_cfg.ini in the path /mnt/data | Implement pathways VI(Offline)-VPSS(Offline)-VO rotate in vi and vpss modify aspect ratio | NULL               |
| VI (Offline)-VPSS(Offline, Rotation)-VO                  | ./sample_vio 3   sensor and panel required  need to place the sensor_cfg.ini in the path /mnt/data | Implement pathways VI(Offline)-VPSS(Offline)-VO and rotate in vpss                        | NULL               |
| VPSS(Offline, file read/write)                           | ./sample_vio 4                                                                                     | Using VPSS, process the input image and save it to the output image                       | output             |
| VI (Two devs)-VPSS-VO                                    | ./sample_vio 5   sensor and panel required  need to place the sensor_cfg.ini in the path /mnt/data | Implement pathways VI-VPSS-VO and there are two vi devices                                | NULL               |
| VPSS(Offline, file read/write, combine 2 frame into 1)   | ./sample_vio 6                                                                                     | VPSS horizontal stitching image                                                           | output             |

