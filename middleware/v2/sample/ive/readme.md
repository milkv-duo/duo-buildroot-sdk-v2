## IVE sample使用说明



| sample                 | arg1  | arg2   | arg3              | arg4              | example                                                      | purpose                                                      |
| ---------------------- | ----- | ------ | ----------------- | ----------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| sample_16bitto8bit     | Width | Height | input image path  | none              | ./sample_16bitto8bit 352 288 data/00_704x576.s16         | 确保16bit to 8bit 功能正常                                   |
| sample_add             | Width | Height | input image path  | none              | ./sample_add 352 288 data/00_352x288_y.yuv                   | 确保add功能正常                                              |
| sample_and             | Width | Height | input image path  | none              | ./sample_and 352 288 data/00_352x288_y.yuv                   | 确保and功能正常                                              |
| sample_bernsen         | Width | Height | input image path  | none              | ./sample_bernsen 352 288 data/00_352x288_y.yuv               | 确保bernsec功能正常                                          |
| sample_bgmodel         | Width | Height | input image path  | none              | ./sample_bgmodel 352 288 data/campus.u8c1.1_100.raw          | 确保bgmodel功能正常                                          |
| sample_cannyedge       | Width | Height | input image path  | none              | ./sample_cannyedge 352 288 data/00_352x288_y.yuv             | ./sample_bernsen 352 288 data/00_352x288_y.yuv确保cannyedge功能正常 |
| sample_cannyhysedge    | Width | Height | input image path  | none              | ./sample_cannyhysedge 352 288 data/00_352x288_y.yuv          | 确保cannyhysedge功能正常                                     |
| sample_csc             | Width | Height | input image path  | none              | ./sample_csc 352 288 data/00_352x288_y.yuv                   | 确保csc功能正常                                              |
| sample_dilate          | Width | Height | input image path  | none              | ./sample_dilate 352 288 data/bin_352x288_y.yuv               | 确保dilate功能正常                                           |
| sample_dma             | Width | Height | input image path  | none              | ./sample_dma 352 288 data/00_352x288_y.yuv                   | 确保dma功能正常                                              |
| sample_erode           | Width | Height | input image path  | none              | ./sample_erode 352 288 data/bin_352x288_y.yuv                | 确保erode功能正常                                            |
| sample_filter          | Width | Height | input image path  | none              | ./sample_filter 352 288 data/00_352x288_y.yuv                | 确保filter功能正常                                           |
| sample_filterandcsc    | Width | Height | input image path  | none              | ./sample_filterandcsc 352 288 data/00_352x288_SP420.yuv      | 确保filterandcsc功能正常                                     |
| sample_framediffmotion | Width | Height | input image1 path | input image2 path | ./sample_framediffmotion 480 480 data/md1_480x480.yuv data/md2_480x480.yuv | 确保framediffmotion功能正常                                  |
| sample_gmm             | Width | Height | input image path  | none              | ./sample_gmm 352 288 data/campus.u8c1.1_100.raw                    | 确保gmm功能正常                                              |
| sample_gmm2            | Width | Height | input image path  | modelnum              | ./sample_gmm2 352 288 data/campus.u8c1.1_100.raw  1             | 确保gmm2功能正常                                             |
| sample_gradfg          | Width | Height | input image path  | none              | ./sample_gradfg 352 288 data/00_352x288_y.yuv                | 确保gradfg功能正常                                           |
| sample_hist            | Width | Height | input image path  | none              | ./sample_hist 352 288 data/00_352x288_y.yuv                  | 确保hist功能正常                                             |
| sample_integ           | Width | Height | input image path  | none              | ./sample_integ 352 288 data/00_352x288_y.yuv                 | 确保integ功能正常                                            |
| sample_lbp             | Width | Height | input image path  | none              | ./sample_lbp 352 288 data/00_352x288_y.yuv                   | 确保lbp功能正常                                              |
| sample_magandang       | Width | Height | input image path  | none              | ./sample_magandang 352 288 data/00_352x288_y.yuv             | 确保magandang功能正常                                        |
| sample_map             | Width | Height | input image path  | none              | ./sample_map 352 288 data/00_352x288_y.yuv                   | 确保map功能正常                                              |
| sample_ncc             | Width | Height | input image1 path | input image2 path | ./sample_ncc 352 288 data/00_352x288_y.yuv data/01_352x288_y.yuv | 确保ncc功能正常                                              |
| sample_normgrad        | Width | Height | input image path  | none              | ./sample_normgrad 352 288 data/00_352x288_y.yuv              | 确保normgrad功能正常                                         |
| sample_or              | Width | Height | input image path  | none              | ./sample_or 352 288 data/00_352x288_y.yuv                    | 确保or功能正常                                               |
| sample_ordstatfilter   | Width | Height | input image path  | none              | ./sample_ordstatfilter 352 288 data/00_352x288_y.yuv         | 确保ordstatfilter功能正常                                    |
| sample_query           | none  | none   | none              | none              | ./sample_query                                               | 确保query功能正常                                            |
| sample_resize          | Width | Height | input image path  | none              | ./sample_resize 352 288 data/campus_352x288.rgb              | 确保resize功能正常                                           |
| sample_sad             | Width | Height | input image1 path | input image2 path | ./sample_sad 352 288 data/00_352x288_y.yuv data/bin_352x288_y.yuv | 确保sad功能正常                                              |
| sample_sobel           | Width | Height | input image path  | none              | ./sample_sobel 352 288 data/00_352x288_y.yuv                 | 确保sobel功能正常                                            |
| sample_stcandicorner   | Width | Height | input image path  | none              | ./sample_stcandicorner 352 288 data/penguin_352x288.gray.shitomasi.raw | 确保stcandicorner功能正常                                    |
| sample_sub             | Width | Height | input image path  | none              | ./sample_sub 352 288 data/00_352x288_y.yuv                   | 确保sub功能正常                                              |
| sample_thresh_S16      | Width | Height | input image path  | none              | ./sample_thresh_S16 352 288 data/00_704x576.s16              | 确保thresh_S16功能正常                                       |
| sample_thresh_U16      | Width | Height | input image path  | none              | ./sample_thresh_S16 352 288 data/00_704x576.u16              | 确保thresh_U16功能正常                                       |
| sample_thresh          | Width | Height | input image path  | none              | ./sample_thresh 352 288 data/00_352x288_y.yuv                | 确保thresh功能正常                                           |
| sample_xor             | Width | Height | input image path  | none              | ./sample_xor 352 288 data/00_352x288_y.yuv                   | 确保xor功能正常                                              |
|                        |       |        |                   |                   |                                                              |                                                              |
|                        |       |        |                   |                   |                                                              |                                                              |

