## sensor_test sample instructions



| sample                                                   | usage                                                                                              | purpose                                                                                   | res                |
| -------------------------------------------------------- | -------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------- | ------------------ |
| dump vi raw frame                                        | ./sample_sensor_test 1   sensor_test required need to place the sensor_cfg.ini in the path /mnt/data |
After running the RGB sensor, grab the original RAW image.                                | NULL               |
| dump vi yuv frame                                        | ./sample_sensor_test 2   sensor_test required need to place the sensor_cfg.ini in the path /mnt/data |
After running RGB or YUV sensor, grab the YUV diagram.                                    | NULL               |
| set chn flip/mirror                                      | ./sample_sensor_test 3   sensor_test required need to place the sensor_cfg.ini in the path /mnt/data |
Flip or mirror the Sensor that supports flip mirroring.                                   | NULL               |
| linear wdr switch                                        | ./sample_sensor_test 4   sensor_test required need to place the sensor_cfg.ini in the path /mnt/data |
Switch between linear and WDR after running the Sensor that supports linear and WDR.      | NULL               |
| AE debug                                                 | ./sample_sensor_test 5   sensor_test required need to place the sensor_cfg.ini in the path /mnt/data |
Detecting the exposure gain of the sensor.                                                | NULL               |
| sensor_test dump                                         | ./sample_sensor_test 6   sensor_test required need to place the sensor_cfg.ini in the path /mnt/data |
Grab the register information of an address.                                              | NULL               |
| sensor_test proc                                         | ./sample_sensor_test 7   sensor_test required need to place the sensor_cfg.ini in the path /mnt/data |
View the current proc information of mipi_rx and vi.                                      | NULL               |
| sensor_test exit                                         | ./sample_sensor_test 255 sensor_test required need to place the sensor_cfg.ini in the path /mnt/data |
Exit sensor test.                                                                         | NULL               |

